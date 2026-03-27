#include "debug_draw.h"
#include "shader.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    float pos[3];
} LineVertex;

bool debug_lines_init_point_light(DebugLines *lines) {
    if (!lines) return false;
    *lines = (DebugLines){0};

    const LineVertex verts[] = {
        {{-1.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f}},
        {{0.0f, -1.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, -1.0f}}, {{0.0f, 0.0f, 1.0f}},
        {{0.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f, 2.5f}},
    };
    lines->vertex_count = (int)(sizeof(verts) / sizeof(verts[0]));

    glGenVertexArrays(1, &lines->vao);
    glGenBuffers(1, &lines->vbo);
    glBindVertexArray(lines->vao);
    glBindBuffer(GL_ARRAY_BUFFER, lines->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                          (void *)offsetof(LineVertex, pos));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

bool debug_lines_init_grid(DebugLines *lines, int half_extent, float spacing) {
    if (!lines || half_extent <= 0 || spacing <= 0.0f) return false;
    *lines = (DebugLines){0};

    int axis_line_count = half_extent * 2 + 1;
    int vertex_count = axis_line_count * 4;
    LineVertex *verts = (LineVertex *)malloc((size_t)vertex_count * sizeof(LineVertex));
    if (!verts) return false;

    float extent = (float)half_extent * spacing;
    int v = 0;
    for (int i = -half_extent; i <= half_extent; i++) {
        float d = (float)i * spacing;
        verts[v++] = (LineVertex){{-extent, 0.0f, d}};
        verts[v++] = (LineVertex){{extent, 0.0f, d}};
        verts[v++] = (LineVertex){{d, 0.0f, -extent}};
        verts[v++] = (LineVertex){{d, 0.0f, extent}};
    }

    lines->vertex_count = vertex_count;
    glGenVertexArrays(1, &lines->vao);
    glGenBuffers(1, &lines->vbo);
    glBindVertexArray(lines->vao);
    glBindBuffer(GL_ARRAY_BUFFER, lines->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)((size_t)vertex_count * sizeof(LineVertex)),
                 verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                          (void *)offsetof(LineVertex, pos));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    free(verts);
    return true;
}

bool debug_lines_init_axis(DebugLines *lines) {
    if (!lines) return false;
    *lines = (DebugLines){0};

    const LineVertex verts[] = {
        {{0.0f, 0.0f, 0.0f}}, {{1.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f}}, {{0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f, 1.0f}},
    };
    lines->vertex_count = (int)(sizeof(verts) / sizeof(verts[0]));

    glGenVertexArrays(1, &lines->vao);
    glGenBuffers(1, &lines->vbo);
    glBindVertexArray(lines->vao);
    glBindBuffer(GL_ARRAY_BUFFER, lines->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                          (void *)offsetof(LineVertex, pos));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void debug_lines_destroy(DebugLines *lines) {
    if (!lines) return;
    if (lines->vao) glDeleteVertexArrays(1, &lines->vao);
    if (lines->vbo) glDeleteBuffers(1, &lines->vbo);
    *lines = (DebugLines){0};
}

void debug_lines_draw(const DebugLines *lines, GLuint shader, const mat4 *view, const mat4 *proj,
                      vec3 position, vec3 direction, float scale, vec3 color) {
    if (!lines || !lines->vao || !view || !proj) return;

    vec3 dir = vec3_normalize(direction);
    float yaw = atan2f(dir.x, dir.z);
    float pitch = -asinf(dir.y);
    mat4 model = mat4_multiply(
        mat4_translate(position),
        mat4_multiply(
            mat4_rotate_y(yaw),
            mat4_multiply(mat4_rotate_x(pitch), mat4_scale(vec3_new(scale, scale, scale)))
        )
    );

    shader_use(shader);
    shader_set_mat4(shader, "u_model", &model);
    shader_set_mat4(shader, "u_view", view);
    shader_set_mat4(shader, "u_proj", proj);
    shader_set_vec3(shader, "u_pick_color", color);

    glBindVertexArray(lines->vao);
    glDrawArrays(GL_LINES, 0, lines->vertex_count);
    glBindVertexArray(0);
}

void debug_lines_draw_world(const DebugLines *lines, GLuint shader, const mat4 *view, const mat4 *proj,
                            vec3 color) {
    if (!lines || !lines->vao || !view || !proj) return;
    mat4 model = mat4_identity();
    shader_use(shader);
    shader_set_mat4(shader, "u_model", &model);
    shader_set_mat4(shader, "u_view", view);
    shader_set_mat4(shader, "u_proj", proj);
    shader_set_vec3(shader, "u_pick_color", color);
    glBindVertexArray(lines->vao);
    glDrawArrays(GL_LINES, 0, lines->vertex_count);
    glBindVertexArray(0);
}

void debug_lines_draw_axis_indicator(const DebugLines *lines, GLuint shader, const mat4 *view,
                                     int window_w, int window_h) {
    if (!lines || !lines->vao || !view || window_w <= 0 || window_h <= 0) return;

    const int gizmo_size = 100;
    mat4 model = mat4_scale(vec3_new(0.8f, 0.8f, 0.8f));
    mat4 view_rot = *view;
    view_rot.m[12] = 0.0f;
    view_rot.m[13] = 0.0f;
    view_rot.m[14] = 0.0f;
    mat4 proj = mat4_ortho(-1.2f, 1.2f, -1.2f, 1.2f, -4.0f, 4.0f);

    glViewport(0, 0, gizmo_size, gizmo_size);
    glDisable(GL_DEPTH_TEST);
    shader_use(shader);
    shader_set_mat4(shader, "u_model", &model);
    shader_set_mat4(shader, "u_view", &view_rot);
    shader_set_mat4(shader, "u_proj", &proj);
    glBindVertexArray(lines->vao);

    shader_set_vec3(shader, "u_pick_color", vec3_new(1.0f, 0.2f, 0.2f));
    glDrawArrays(GL_LINES, 0, 2);
    shader_set_vec3(shader, "u_pick_color", vec3_new(0.2f, 1.0f, 0.2f));
    glDrawArrays(GL_LINES, 2, 2);
    shader_set_vec3(shader, "u_pick_color", vec3_new(0.3f, 0.5f, 1.0f));
    glDrawArrays(GL_LINES, 4, 2);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, window_w, window_h);
}
