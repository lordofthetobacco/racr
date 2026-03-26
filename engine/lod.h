#ifndef ENGINE_LOD_H
#define ENGINE_LOD_H

#include "mesh.h"

#define LOD_LEVEL_MAX 3

typedef struct {
    Mesh mesh;
    int index_count;
    float max_distance;
} LODLevel;

void lod_levels_destroy(LODLevel *levels, int count);
int lod_generate_levels(const Vertex *verts, int nv,
                        const uint32_t *indices, int ni,
                        LODLevel *out_levels, int max_levels);

#endif
