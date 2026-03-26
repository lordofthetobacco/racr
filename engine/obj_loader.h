#ifndef ENGINE_OBJ_LOADER_H
#define ENGINE_OBJ_LOADER_H

#include "mesh.h"
#include <stdbool.h>
#include <stdint.h>

#define OBJ_GROUP_NAME_MAX 64

typedef struct {
    char name[OBJ_GROUP_NAME_MAX];
    int first_index;
    int index_count;
} ObjGroup;

bool obj_load(const char *path,
              Vertex **out_verts, int *out_nv,
              uint32_t **out_indices, int *out_ni);
bool obj_load_grouped(const char *path,
                      Vertex **out_verts, int *out_nv,
                      uint32_t **out_indices, int *out_ni,
                      ObjGroup **out_groups, int *out_group_count);

#endif
