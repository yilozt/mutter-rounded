#include "meta_clip_effect.h"
#include "meta/prefs.h"

typedef struct {
  CoglPipeline *pipeline;
  ClutterActor *actor;
  cairo_rectangle_int_t bounds;
} MetaClipEffectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(MetaClipEffect, meta_clip_effect, CLUTTER_TYPE_OFFSCREEN_EFFECT)

/*
 * take from src/compositor/meta-background-content.c
 * 
 * The ellipsis_dist(), ellipsis_coverage() and rounded_rect_coverage() are
 * copied from GSK, see gsk_ellipsis_dist(), gsk_ellipsis_coverage(), and
 * gsk_rounded_rect_coverage() here:
 * https://gitlab.gnome.org/GNOME/gtk/-/blob/master/gsk/resources/glsl/preamble.fs.glsl
 *
 */
#define ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS                            \
"uniform vec4 bounds;           // x, y: top left; w, v: bottom right     \n"\
"uniform vec4 corner_centers_1; // x, y: top left; w, v: top right        \n"\
"uniform vec4 corner_centers_2; // x, y: bottom right; w, v: bottom left  \n"\
"uniform vec2 pixel_step;                                                 \n"\
"uniform int skip;                                                        \n"\
"                                                                         \n"\
"float                                                                    \n"\
"ellipsis_dist (vec2 p, vec2 radius)                                      \n"\
"{                                                                        \n"\
"  if (radius == vec2(0, 0))                                              \n"\
"    return 0.0;                                                          \n"\
"                                                                         \n"\
"  vec2 p0 = p / radius;                                                  \n"\
"  vec2 p1 = (2.0 * p0) / radius;                                         \n"\
"                                                                         \n"\
"  return (dot(p0, p0) - 1.0) / length (p1);                              \n"\
"}                                                                        \n"\
"                                                                         \n"\
"float                                                                    \n"\
"ellipsis_coverage (vec2 point, vec2 center, vec2 radius)                 \n"\
"{                                                                        \n"\
"  float d = ellipsis_dist ((point - center), radius);                    \n"\
"  return clamp (0.5 - d, 0.0, 1.0);                                      \n"\
"}                                                                        \n"\
"                                                                         \n"\
"float                                                                    \n"\
"rounded_rect_coverage (vec4 bounds,                                      \n"\
"                       vec4 corner_centers_1,                            \n"\
"                       vec4 corner_centers_2,                            \n"\
"                       vec2 p)                                           \n"\
"{                                                                        \n"\
"  if (p.x < bounds.x || p.y < bounds.y ||                                \n"\
"      p.x >= bounds.z || p.y >= bounds.w)                                \n"\
"    return 0.0;                                                          \n"\
"                                                                         \n"\
"  vec2 ref_tl = corner_centers_1.xy;                                     \n"\
"  vec2 ref_tr = corner_centers_1.zw;                                     \n"\
"  vec2 ref_br = corner_centers_2.xy;                                     \n"\
"  vec2 ref_bl = corner_centers_2.zw;                                     \n"\
"                                                                         \n"\
"  if (p.x >= ref_tl.x && p.x >= ref_bl.x &&                              \n"\
"      p.x <= ref_tr.x && p.x <= ref_br.x)                                \n"\
"    return 1.0;                                                          \n"\
"                                                                         \n"\
"  if (p.y >= ref_tl.y && p.y >= ref_tr.y &&                              \n"\
"      p.y <= ref_bl.y && p.y <= ref_br.y)                                \n"\
"    return 1.0;                                                          \n"\
"                                                                         \n"\
"  vec2 rad_tl = corner_centers_1.xy - bounds.xy;                         \n"\
"  vec2 rad_tr = corner_centers_1.zw - bounds.zy;                         \n"\
"  vec2 rad_br = corner_centers_2.xy - bounds.zw;                         \n"\
"  vec2 rad_bl = corner_centers_2.zw - bounds.xw;                         \n"\
"                                                                         \n"\
"  float d_tl = ellipsis_coverage(p, ref_tl, rad_tl);                     \n"\
"  float d_tr = ellipsis_coverage(p, ref_tr, rad_tr);                     \n"\
"  float d_br = ellipsis_coverage(p, ref_br, rad_br);                     \n"\
"  float d_bl = ellipsis_coverage(p, ref_bl, rad_bl);                     \n"\
"                                                                         \n"\
"  vec4 corner_coverages = 1.0 - vec4(d_tl, d_tr, d_br, d_bl);            \n"\
"                                                                         \n"\
"  bvec4 is_out = bvec4(p.x < ref_tl.x && p.y < ref_tl.y,                 \n"\
"                       p.x > ref_tr.x && p.y < ref_tr.y,                 \n"\
"                       p.x > ref_br.x && p.y > ref_br.y,                 \n"\
"                       p.x < ref_bl.x && p.y > ref_bl.y);                \n"\
"                                                                         \n"\
"  return 1.0 - dot(vec4(is_out), corner_coverages);                      \n"\
"}                                                                        \n"

#define ROUNDED_CLIP_FRAGMENT_SHADER_CODE                                    \
"vec2 texture_coord;                                                      \n"\
"                                                                         \n"\
"texture_coord = cogl_tex_coord0_in.xy / pixel_step;                      \n"\
"                                                                         \n"\
"if (skip == 0)                                                           \n"\
"cogl_color_out *= rounded_rect_coverage (bounds,                         \n"\
"                                         corner_centers_1,               \n"\
"                                         corner_centers_2,               \n"\
"                                         texture_coord);                 \n"\

static CoglPipeline *
meta_clip_effect_class_create_pipeline(ClutterOffscreenEffect *effect,
                                       CoglTexture *texture)
{
  MetaClipEffect *clip_effect = META_CLIP_EFFECT (effect);
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(clip_effect);
  cogl_pipeline_set_layer_texture (priv->pipeline, 0, texture);
  return cogl_object_ref (priv->pipeline);
}

static void
meta_clip_effect_set_actor(ClutterActorMeta *meta,
                           ClutterActor *actor)
{
  ClutterActorMetaClass *meta_class 
    = CLUTTER_ACTOR_META_CLASS(meta_clip_effect_parent_class);
  MetaClipEffectPrivate *priv = 
    meta_clip_effect_get_instance_private(META_CLIP_EFFECT(meta));
  meta_class->set_actor(meta, actor);
  priv->actor = clutter_actor_meta_get_actor(meta);
}

static void
meta_clip_effect_dispose(GObject *gobject)
{
  MetaClipEffect*effect = META_CLIP_EFFECT(gobject);
  MetaClipEffectPrivate *priv = 
    meta_clip_effect_get_instance_private(META_CLIP_EFFECT(effect));

  if (priv->pipeline != NULL)
    g_clear_pointer(&priv->pipeline, cogl_object_unref);

  G_OBJECT_CLASS (meta_clip_effect_parent_class)->dispose (gobject);
}

static void
meta_clip_effect_class_init(MetaClipEffectClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterOffscreenEffectClass *offscreen_class = CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);
  ClutterActorMetaClass *meta_class = CLUTTER_ACTOR_META_CLASS(klass);

  meta_class->set_actor = meta_clip_effect_set_actor;
  offscreen_class->create_pipeline = meta_clip_effect_class_create_pipeline;
  gobject_class->dispose = meta_clip_effect_dispose;
}

static void
meta_clip_effect_init(MetaClipEffect *self)
{
  MetaClipEffectClass *klass = META_CLIP_EFFECT_GET_CLASS (self);
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(self);

  if (G_UNLIKELY (klass->base_pipeline == NULL))
    {
      CoglSnippet *snippet;
      CoglContext *ctx =
        clutter_backend_get_cogl_context (clutter_get_default_backend ());

      klass->base_pipeline = cogl_pipeline_new (ctx);

      snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT,
                                  ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS,
                                  ROUNDED_CLIP_FRAGMENT_SHADER_CODE);
      cogl_pipeline_add_snippet (klass->base_pipeline, snippet);
      cogl_object_unref (snippet);

      cogl_pipeline_set_layer_null_texture (klass->base_pipeline, 0);
    }

  priv->pipeline = cogl_pipeline_copy (klass->base_pipeline);
  priv->actor = NULL;
}

MetaClipEffect *meta_clip_effect_new(void)
{
  return g_object_new(META_TYPE_CLIP_EFFECT, NULL);
}

void
meta_clip_effect_set_bounds(MetaClipEffect *effect, 
                              cairo_rectangle_int_t *_bounds)
{
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);

  g_return_if_fail(priv->pipeline && priv->actor);
  float padding = meta_prefs_get_window_edge_padding();
  float radius = meta_prefs_get_round_corner_radius();

  priv->bounds.x = _bounds->x + padding + 1;
  priv->bounds.y = _bounds->y + padding + 1;
  priv->bounds.width =  _bounds->width  - 2 * padding - 1;
  priv->bounds.height = _bounds->height - 2 * padding - 1;

  float x1 = priv->bounds.x;
  float y1 = priv->bounds.y;
  float x2 = priv->bounds.width + x1;
  float y2 = priv->bounds.height + y1;
  float w, h;

  clutter_actor_get_size(priv->actor, &w, &h);

  int location_skip = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "skip");
  int location_bounds = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "bounds");
  int location_corner_centers_1 =
    cogl_pipeline_get_uniform_location(priv->pipeline, "corner_centers_1");
  int location_corner_centers_2 =
    cogl_pipeline_get_uniform_location(priv->pipeline, "corner_centers_2");
  int location_pixel_step =
    cogl_pipeline_get_uniform_location(priv->pipeline, "pixel_step");

  float bounds[] = { x1, y1, x2, y2 };
  float corner_centers_1[] = {
    x1 + radius,
    y1 + radius,
    x2 - radius,
    y1 + radius
  };
  float corner_centers_2[] = {
    x2 - radius,
    y2 - radius,
    x1 + radius,
    y2 - radius
  };
  float pixel_step[] = { 1. / w, 1. / h };

  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_bounds,
                                  4, 1, bounds);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_corner_centers_1,
                                  4, 1, corner_centers_1);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_corner_centers_2,
                                  4, 1, corner_centers_2);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_pixel_step,
                                  2, 1, pixel_step);
  cogl_pipeline_set_uniform_1i(priv->pipeline, location_skip, 0);
}

void meta_clip_effect_skip(MetaClipEffect *effect)
{
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);

  g_return_if_fail(priv->pipeline && priv->actor);

  int location_skip = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "skip");

  cogl_pipeline_set_uniform_1i(priv->pipeline, location_skip, 1);
}

void meta_clip_effect_get_bounds(MetaClipEffect *effect, cairo_rectangle_int_t *bounds)
{
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);
  *bounds = priv->bounds;
}