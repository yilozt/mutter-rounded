#pragma once

/*
 * copied from src/compositor/meta-background-content.c
 * 
 * The ellipsis_dist(), ellipsis_coverage() and rounded_rect_coverage() are
 * copied from GSK, see gsk_ellipsis_dist(), gsk_ellipsis_coverage(), and
 * gsk_rounded_rect_coverage() here:
 * https://gitlab.gnome.org/GNOME/gtk/-/blob/master/gsk/resources/glsl/preamble.fs.glsl
 *
 */
#define ROUNDED_CLIP_FRAGMENT_SHADER_FUNCS                                   \
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

#define ROUNDED_CLIP_FRAGMENT_SHADER_VARS                                    \
"uniform vec4 bounds;           // x, y: top left; w, v: bottom right     \n"\
"uniform vec4 corner_centers_1; // x, y: top left; w, v: top right        \n"\
"uniform vec4 corner_centers_2; // x, y: bottom right; w, v: bottom left  \n"\
"uniform vec4 inner_bounds;                                               \n"\
"uniform vec4 inner_corner_centers_1;                                     \n"\
"uniform vec4 inner_corner_centers_2;                                     \n"\
"uniform vec2 pixel_step;                                                 \n"\
"uniform int skip;                                                        \n"\
"uniform float border_width;                                              \n"

/* used by src/meta_clip_effect.c  */
#define ROUNDED_CLIP_FRAGMENT_SHADER_DECLARATIONS                            \
ROUNDED_CLIP_FRAGMENT_SHADER_VARS                                            \
ROUNDED_CLIP_FRAGMENT_SHADER_FUNCS

/* used by src/meta_clip_effect.c  */
#define ROUNDED_CLIP_FRAGMENT_SHADER_CODE                                    \
"if (skip == 0) {                                                         \n"\
"  vec2 texture_coord = cogl_tex_coord0_in.xy / pixel_step;               \n"\
"                                                                         \n"\
"  float outer_alpha = rounded_rect_coverage (bounds,                     \n"\
"                                             corner_centers_1,           \n"\
"                                             corner_centers_2,           \n"\
"                                             texture_coord);             \n"\
"  if (border_width > 0.0) {                                              \n"\
"    float inner_alpha = rounded_rect_coverage (inner_bounds,             \n"\
"                                               inner_corner_centers_1,   \n"\
"                                               inner_corner_centers_2,   \n"\
"                                               texture_coord);           \n"\
"    float border_alpha = clamp (outer_alpha - inner_alpha, 0.0, 1.0)     \n"\
"                       * cogl_color_out.a;                               \n"\
"                                                                         \n"\
"    cogl_color_out *= smoothstep (0.0, 0.6, inner_alpha);                \n"\
"    cogl_color_out = mix (cogl_color_out,                                \n"\
"                          vec4(vec3(0.65), 1.0),                         \n"\
"                          border_alpha);                                 \n"\
"  } else {                                                               \n"\
"    cogl_color_out = cogl_color_out * outer_alpha;                       \n"\
"  }                                                                      \n"\
"}                                                                        \n"

#define ROUNDED_CLIP_FRAGMENT_SHADER_VARS_BLUR                               \
"uniform vec4 bounds;           // x, y: top left; w, v: bottom right     \n"\
"uniform vec4 corner_centers_1; // x, y: top left; w, v: top right        \n"\
"uniform vec4 corner_centers_2; // x, y: bottom right; w, v: bottom left  \n"\
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
"  cogl_color_out *= rounded_rect_coverage (bounds,                       \n"\
"                                           corner_centers_1,             \n"\
"                                           corner_centers_2,             \n"\
"                                           texture_coord);               \n"\
"}                                                                        \n"\
"cogl_color_out.rgb *= brightness;                                        \n"
