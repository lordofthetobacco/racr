#include "debug_draw.h"
#include "shader.h"
#include <math.h>
#include <stddef.h>

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
