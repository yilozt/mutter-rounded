#pragma once

/*
 * copied from src/compositor/meta-background-content.c
 * see: https://gitlab.gnome.org/GNOME/mutter/-/blob/858b5c12b1f55043964c2e2bd30de8cf112e76d2/src/compositor/meta-background-content.c#L138
 */
#define ROUNDED_CLIP_FRAGMENT_SHADER_FUNCS                                   \
"float                                                                    \n"\
"rounded_rect_coverage (vec2 p, vec4 bounds, float clip_radius)           \n"\
"{                                                                        \n"\
"  // Outside the bounds                                                  \n"\
"  if (p.x < bounds.x || p.x > bounds.z                                   \n"\
"     || p.y < bounds.y || p.y > bounds.w ) {                             \n"\
"    return 0.0;                                                          \n"\
"  }                                                                      \n"\
"  float center_left  = bounds.x + clip_radius;                           \n"\
"  float center_right = bounds.z - clip_radius;                           \n"\
"  float center_x;                                                        \n"\
"                                                                         \n"\
"  if (p.x < center_left)                                                 \n"\
"    center_x = center_left;                                              \n"\
"  else if (p.x > center_right)                                           \n"\
"    center_x = center_right;                                             \n"\
"  else                                                                   \n"\
"    return 1.0; // The vast majority of pixels exit early here           \n"\
"                                                                         \n"\
"  float center_top    = bounds.y + clip_radius;                          \n"\
"  float center_bottom = bounds.w - clip_radius;                          \n"\
"  float center_y;                                                        \n"\
"                                                                         \n"\
"  if (p.y < center_top)                                                  \n"\
"    center_y = center_top;                                               \n"\
"  else if (p.y > center_bottom)                                          \n"\
"    center_y = center_bottom;                                            \n"\
"  else                                                                   \n"\
"    return 1.0;                                                          \n"\
"                                                                         \n"\
"  vec2 delta = p - vec2 (center_x, center_y);                            \n"\
"  float dist_squared = dot (delta, delta);                               \n"\
"                                                                         \n"\
"  // Fully outside the circle                                            \n"\
"  float outer_radius = clip_radius + 0.5;                                \n"\
"  if (dist_squared >= (outer_radius * outer_radius))                     \n"\
"    return 0.0;                                                          \n"\
"                                                                         \n"\
"  // Fully inside the circle                                             \n"\
"  float inner_radius = clip_radius - 0.5;                                \n"\
"  if (dist_squared <= (inner_radius * inner_radius))                     \n"\
"    return 1.0;                                                          \n"\
"                                                                         \n"\
"                                                                         \n"\
"  // Only pixels on the edge of the curve need expensive antialiasing    \n"\
"  return outer_radius - sqrt (dist_squared);                             \n"\
"}                                                                        \n"

#define ROUNDED_CLIP_FRAGMENT_SHADER_VARS                                    \
"uniform vec4 bounds;           // x, y: top left; z, w: bottom right     \n"\
"uniform float clip_radius;                                               \n"\
"uniform vec4 inner_bounds;                                               \n"\
"uniform float inner_clip_radius;                                         \n"\
"uniform vec2 pixel_step;                                                 \n"\
"uniform int skip;                                                        \n"\
"uniform float border_width;                                              \n"\
"uniform float border_brightness;                                         \n"

/* used by src/meta_clip_effect.c  */
#define ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS                            \
ROUNDED_CLIP_FRAGMENT_SHADER_VARS                                            \
ROUNDED_CLIP_FRAGMENT_SHADER_FUNCS

/* used by src/meta_clip_effect.c  */
#define ROUNDED_CLIP_FRAGMENT_SHADER_CODE                                    \
"if (skip == 0) {                                                         \n"\
"  vec2 texture_coord = cogl_tex_coord0_in.xy / pixel_step;               \n"\
"                                                                         \n"\
"  float outer_alpha = rounded_rect_coverage (texture_coord,              \n"\
"                                             bounds,                     \n"\
"                                             clip_radius);               \n"\
"  if (border_width > 0.0) {                                              \n"\
"    float inner_alpha = rounded_rect_coverage (texture_coord,            \n"\
"                                               inner_bounds,             \n"\
"                                               inner_clip_radius);       \n"\
"    float border_alpha = clamp (outer_alpha - inner_alpha, 0.0, 1.0)     \n"\
"                       * cogl_color_out.a;                               \n"\
"                                                                         \n"\
"    cogl_color_out *= smoothstep (0.0, 0.6, inner_alpha);                \n"\
"    cogl_color_out = mix (cogl_color_out,                                \n"\
"                          vec4(vec3(border_brightness), 1.0),            \n"\
"                          border_alpha);                                 \n"\
"  } else {                                                               \n"\
"    cogl_color_out = cogl_color_out * outer_alpha;                       \n"\
"  }                                                                      \n"\
"}                                                                        \n"

#define ROUNDED_CLIP_FRAGMENT_SHADER_VARS_BLUR                               \
"uniform vec4 bounds;           // x, y: top left; w, v: bottom right     \n"\
"uniform float clip_radius;                                               \n"\
"uniform vec2 pixel_step;                                                 \n"\
"uniform int skip;                                                        \n"\
"uniform float brightness;                                                \n"

/* used by shell-blur-effect.c */
#define ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS_BLUR                       \
ROUNDED_CLIP_FRAGMENT_SHADER_VARS_BLUR                                       \
ROUNDED_CLIP_FRAGMENT_SHADER_FUNCS

/* used by shell-blur-effect.c */
#define ROUNDED_CLIP_FRAGMENT_SHADER_CODE_BLUR                               \
"if (skip == 0) {                                                         \n"\
"  vec2 texture_coord = cogl_tex_coord0_in.xy / pixel_step;               \n"\
"                                                                         \n"\
"  cogl_color_out *= rounded_rect_coverage (texture_coord,                \n"\
"                                           bounds,                       \n"\
"                                           clip_radius);                 \n"\
"}                                                                        \n"\
"cogl_color_out.rgb *= brightness;                                        \n"
