// for 40.4

#pragma once

#include <clutter/clutter.h>

#define META_TYPE_CLIP_EFFECT (meta_clip_effect_get_type())
G_DECLARE_DERIVABLE_TYPE(MetaClipEffect, meta_clip_effect, META, CLIP_EFFECT, ClutterOffscreenEffect)

struct _MetaClipEffectClass {
  ClutterOffscreenEffectClass parent_class;
  CoglPipeline *base_pipeline;
  gpointer padding[12];
};

MetaClipEffect *meta_clip_effect_new(void);

void meta_clip_effect_set_bounds(MetaClipEffect *effect, cairo_rectangle_int_t *bounds, int padding[4]);
void meta_clip_effect_get_bounds(MetaClipEffect *effect, cairo_rectangle_int_t *bounds);
void meta_clip_effect_skip(MetaClipEffect *effect);