#ifndef ENGINE_DEBUG_DRAW_H
#define ENGINE_DEBUG_DRAW_H

#include "gl_funcs.h"
#include "math_types.h"
#include <stdbool.h>

typedef struct {
    GLuint vao;
    GLuint vbo;
    int vertex_count;
} DebugLines;

bool debug_lines_init_point_light(DebugLines *lines);
bool debug_lines_init_grid(DebugLines *lines, int half_extent, float spacing);
bool debug_lines_init_axis(DebugLines *lines);
void debug_lines_destroy(DebugLines *lines);
void debug_lines_draw(const DebugLines *lines, GLuint shader, const mat4 *view, const mat4 *proj,
                      vec3 position, vec3 direction, float scale, vec3 color);
void debug_lines_draw_world(const DebugLines *lines, GLuint shader, const mat4 *view, const mat4 *proj,
                            vec3 color);
void debug_lines_draw_axis_indicator(const DebugLines *lines, GLuint shader, const mat4 *view,
                                     int window_w, int window_h);

#endif
