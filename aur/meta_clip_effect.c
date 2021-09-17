/**
 * for mutter 40.4
 */

#include "meta_clip_effect.h"
#include "meta/prefs.h"

typedef struct {
  CoglPipeline *pipeline;
  CoglTexture *corner_texture;
  ClutterActor *actor;
  cairo_rectangle_int_t bounds;
} MetaClipEffectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(MetaClipEffect, meta_clip_effect, CLUTTER_TYPE_OFFSCREEN_EFFECT)

/*
 * THIS shader copied from the kwin-plugin of CutefishOS project
 * see https://github.com/cutefishos/kwin-plugins/blob/main/plugins/roundedwindow/roundedwindow.cpp
 */
#define ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS                            \
"uniform vec2 corner_scale;                                                             \n"\
"uniform vec2 offset_lt;   // left top                                                  \n"\
"uniform vec2 offset_rt;   // right top                                                 \n"\
"uniform vec2 offset_lb;   // left bottom                                               \n"\
"uniform vec2 offset_rb;   // right bottom                                              \n"\
"uniform vec4 bounds;                                                                   \n"\
"uniform float opacity;                                                                 \n"\
"uniform int skip;                                                                      \n"\
"                                                                                       \n"\
"float cal_alpha(void) {                                                                \n"\
"  vec2 scale = corner_scale;                                                           \n"\
"  vec2 coord = cogl_tex_coord0_in.xy;                                                  \n"\
"  vec4 fg;                                                                             \n"\
"                                                                                       \n"\
"  if(coord.x < bounds.x || coord.y < bounds.y ||                                       \n"\
"    coord.x > bounds.z || coord.y > bounds.w) {                                        \n"\
"    return 0.0;                                                                        \n"\
"  }                                                                                    \n"\
"                                                                                       \n"\
"  if(coord.x < 0.5) {                                                                  \n"\
"    if(coord.y < 0.5) {                                                                \n"\
"      vec2 cornerCoord = vec2(coord.x * scale.x, coord.y * scale.y);                   \n"\
"      fg = texture2D(cogl_sampler1, cornerCoord - offset_lt);                          \n"\
"    } else {                                                                           \n"\
"      vec2 cornerCoordBL = vec2(coord.x * scale.x, (1.0 - coord.y) * scale.y);         \n"\
"      fg = texture2D(cogl_sampler1, cornerCoordBL - offset_lb);                        \n"\
"    }                                                                                  \n"\
"  } else {                                                                             \n"\
"    if(coord.y < 0.5) {                                                                \n"\
"      vec2 cornerCoordTR = vec2((1.0 - coord.x) * scale.x, coord.y * scale.y);         \n"\
"      fg = texture2D(cogl_sampler1, cornerCoordTR - offset_rt);                        \n"\
"    } else {                                                                           \n"\
"      vec2 cornerCoordBR = vec2((1.0 - coord.x) * scale.x, (1.0 - coord.y) * scale.y); \n"\
"      fg = texture2D(cogl_sampler1, cornerCoordBR - offset_rb);                        \n"\
"    }                                                                                  \n"\
"  }                                                                                    \n"\
"                                                                                       \n"\
"  return fg.r;                                                                         \n"\
"}                                                                                      \n"

#define ROUNDED_CLIP_FRAGMENT_SHADER_CODE                                    \
"vec4 bg = texture2D(cogl_sampler0, cogl_tex_coord0_in.xy) * opacity;                  \n"\
"if (skip == 1)\n"\
"  cogl_color_out = bg;                      \n"\
"else\n"\
"  cogl_color_out = bg * cal_alpha();                      \n"
// "cogl_color_out = bg * 0.6 + (vec4(0.4) * cal_alpha());                      \n"



static CoglPipeline *
meta_clip_effect_class_create_pipeline(ClutterOffscreenEffect *effect,
                                       CoglTexture            *texture)
{
  MetaClipEffect *clip_effect = META_CLIP_EFFECT (effect);
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(clip_effect);
  cogl_pipeline_set_layer_texture (priv->pipeline, 0, texture);
  cogl_pipeline_set_layer_texture(priv->pipeline, 1, priv->corner_texture);

  return cogl_object_ref (priv->pipeline);
}

static void
meta_clip_effect_set_actor(ClutterActorMeta *meta,
                           ClutterActor     *actor)
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
  {
    g_clear_pointer(&priv->pipeline, cogl_object_unref);
  }

  G_OBJECT_CLASS (meta_clip_effect_parent_class)->dispose (gobject);
}

static
CoglTexture *gen_texture(void)
{
  int radius = meta_prefs_get_round_corner_radius();
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, radius);
  uint8_t *pixel = g_malloc0(stride * radius);
  ClutterBackend *backend = clutter_get_default_backend ();
  CoglContext *ctx = clutter_backend_get_cogl_context (backend);
  cairo_surface_t *image = 
    cairo_image_surface_create_for_data(pixel,
                                        CAIRO_FORMAT_ARGB32,
                                        radius,
                                        radius,
                                        stride);
  cairo_t *cr = cairo_create(image);

  /* draw a 1 / 4 circel, a small texture sifed with radius x radius
   * texture will be look like this:
   * 
   *                 XXXX
   *              XXXXXXX
   *            XXXXXXXXX
   *           XXXXXXXXXX
   *          XXXXXXXXXXX
   *          XXXXXXXXXXX
   *  
   */

  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
  cairo_fill(cr);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_move_to(cr, 0, radius);
  cairo_arc(cr, radius, radius, radius, M_PI, 1.5 * M_PI);
  cairo_line_to(cr, radius, radius);
  cairo_line_to(cr, 0, radius);
  cairo_close_path (cr);
  cairo_fill(cr);

  cairo_surface_flush(image);

  GError *error = NULL;
  CoglTexture2D *texture =
    cogl_texture_2d_new_from_data(ctx, radius, radius,
                                  COGL_PIXEL_FORMAT_ARGB_8888,
                                  stride, pixel, &error);
  if (error)
  {
    g_warning ("Failed to allocate mask texture: %s", error->message);
    g_error_free (error);
    g_clear_pointer(&texture, cogl_object_unref);
  }
  g_free(pixel);
  cairo_destroy(cr);
  cairo_surface_destroy (image);
  return COGL_TEXTURE(texture);
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
      klass->base_corner_texture = gen_texture();

      snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT,
                                  ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS,
                                  ROUNDED_CLIP_FRAGMENT_SHADER_CODE);
      cogl_pipeline_add_snippet (klass->base_pipeline, snippet);
      cogl_object_unref (snippet);

      cogl_pipeline_set_layer_null_texture (klass->base_pipeline, 0);
    }

  priv->pipeline = cogl_pipeline_copy (klass->base_pipeline);
  priv->corner_texture = klass->base_corner_texture;
  // g_print("corner ref: %ld\n", COGL_OBJECT(priv->corner_texture)->ref_count);
  priv->actor = NULL;
}

MetaClipEffect *meta_clip_effect_new(void)
{
  return g_object_new(META_TYPE_CLIP_EFFECT, NULL);
}

void
meta_clip_effect_set_bounds(MetaClipEffect        *effect, 
                            cairo_rectangle_int_t *_bounds,
                            int                   padding[4])
{
  // padding: [left, right, top, bottom]

  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);

  g_return_if_fail(priv->pipeline && priv->actor);
  float radius = meta_prefs_get_round_corner_radius();

  priv->bounds.x = _bounds->x + padding[0];
  priv->bounds.y = _bounds->y + padding[2];
  priv->bounds.width =  _bounds->width  - padding[1] - padding[0];
  priv->bounds.height = _bounds->height - padding[2] - padding[3];
  float w, h;

  clutter_actor_get_size(priv->actor, &w, &h);

  int location_skip = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "skip");
  int location_opacity = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "opacity");
  int location_offset_lt = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "offset_lt");
  int location_offset_rt =
    cogl_pipeline_get_uniform_location(priv->pipeline, "offset_rt");
  int location_offset_lb =
    cogl_pipeline_get_uniform_location(priv->pipeline, "offset_lb");
  int location_offset_rb =
    cogl_pipeline_get_uniform_location(priv->pipeline, "offset_rb");
  int location_corner_scale = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "corner_scale");
  int location_bounds = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "bounds");

  float bounds[] = {
    priv->bounds.x / w,
    priv->bounds.y / h,
    (priv->bounds.x + priv->bounds.width) / w,
    (priv->bounds.y + priv->bounds.height) / h
  };
  float corner_scale[] = { w / radius , h / radius };
  float top = priv->bounds.y;
  float left = priv->bounds.x;
  float right = w - priv->bounds.width - priv->bounds.x;
  float bottom = h - priv->bounds.height - priv->bounds.y;

  float offset_lt[] = { left / (float) radius, top / (float)radius };
  float offset_rt[] = { right / (float) radius, top / (float)radius };
  float offset_lb[] = { left / (float) radius, bottom / (float)radius };
  float offset_rb[] = { right / (float) radius, bottom / (float)radius };

  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_offset_lt, 2, 1, offset_lt);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_offset_rt, 2, 1, offset_rt);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_offset_lb, 2, 1, offset_lb);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_offset_rb, 2, 1, offset_rb);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_corner_scale, 2, 1, corner_scale);
  cogl_pipeline_set_uniform_float(priv->pipeline,
                                  location_bounds, 4, 1, bounds);
  cogl_pipeline_set_uniform_1f(priv->pipeline,
                               location_opacity,
                               clutter_actor_get_opacity(priv->actor) / 255.0);
  cogl_pipeline_set_uniform_1i(priv->pipeline, location_skip, 0);
}

void
meta_clip_effect_skip(MetaClipEffect *effect)
{
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);

  g_return_if_fail(priv->pipeline && priv->actor);

  int location_skip = 
    cogl_pipeline_get_uniform_location(priv->pipeline, "skip");

  cogl_pipeline_set_uniform_1i(priv->pipeline, location_skip, 1);
}

void
meta_clip_effect_get_bounds(MetaClipEffect        *effect,
                            cairo_rectangle_int_t *bounds)
{
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);
  *bounds = priv->bounds;
}

void
meta_clip_effect_update_corner_texture(MetaClipEffect *effect)
{
  MetaClipEffectClass *klass = META_CLIP_EFFECT_GET_CLASS(effect);
  MetaClipEffectPrivate *priv = meta_clip_effect_get_instance_private(effect);

  if (klass->base_corner_texture == priv->corner_texture)
  {
    CoglTexture *new_texture = gen_texture();
    if (new_texture != NULL)
    {
      cogl_object_unref(klass->base_corner_texture);
      klass->base_corner_texture = new_texture;
      priv->corner_texture = klass->base_corner_texture;

      cogl_pipeline_set_layer_texture(priv->pipeline, 1, priv->corner_texture);
    }
  } else {
    // it's need-not call `cogl_object_unref` to the old texture
    priv->corner_texture = klass->base_corner_texture;
    // because this function will call `cogl_object_unref` to the old texture
    // at the same time, this function will add the reference count of new texture
    cogl_pipeline_set_layer_texture(priv->pipeline, 1, priv->corner_texture);
  }
}