#ifndef ENGINE_OBJ_LOADER_H
#define ENGINE_OBJ_LOADER_H

#include "mesh.h"
#include <stdbool.h>
#include <stdint.h>

bool obj_load(const char *path,
              Vertex **out_verts, int *out_nv,
              uint32_t **out_indices, int *out_ni);

#endif
