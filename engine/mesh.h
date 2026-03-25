#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include "gl_funcs.h"
#include <stdint.h>

typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
} Vertex;

typedef struct {
    GLuint vao, vbo, ebo;
    int index_count;
} Mesh;

Mesh  mesh_create(const Vertex *verts, int nv,
                  const uint32_t *indices, int ni);
void  mesh_draw(const Mesh *m);
void  mesh_destroy(Mesh *m);

#endif
