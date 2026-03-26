#include "lod.h"
#include <stdlib.h>
#include <string.h>

static Mesh create_decimated_mesh(const Vertex *verts, int nv,
                                  const uint32_t *indices, int ni,
                                  int keep_step, int *out_index_count) {
    int tri_count = ni / 3;
    int kept_tris = 0;
    for (int t = 0; t < tri_count; t++) {
        if ((t % keep_step) == 0) kept_tris++;
    }
    int new_ni = kept_tris * 3;
    *out_index_count = new_ni;
    if (new_ni <= 0) return (Mesh){0};

    uint32_t *new_indices = malloc((size_t)new_ni * sizeof(uint32_t));
    if (!new_indices) return (Mesh){0};

    int wr = 0;
    for (int t = 0; t < tri_count; t++) {
        if ((t % keep_step) != 0) continue;
        int base = t * 3;
        new_indices[wr++] = indices[base];
        new_indices[wr++] = indices[base + 1];
        new_indices[wr++] = indices[base + 2];
    }

    Mesh m = mesh_create(verts, nv, new_indices, new_ni);
    free(new_indices);
    return m;
}

void lod_levels_destroy(LODLevel *levels, int count) {
    if (!levels) return;
    for (int i = 0; i < count; i++) {
        mesh_destroy(&levels[i].mesh);
    }
}

int lod_generate_levels(const Vertex *verts, int nv,
                        const uint32_t *indices, int ni,
                        LODLevel *out_levels, int max_levels) {
    if (!verts || !indices || !out_levels || nv <= 0 || ni < 3 || max_levels <= 0)
        return 0;
    if (max_levels > LOD_LEVEL_MAX) max_levels = LOD_LEVEL_MAX;

    int level_count = 0;
    out_levels[level_count].mesh = mesh_create(verts, nv, indices, ni);
    out_levels[level_count].index_count = ni;
    out_levels[level_count].max_distance = 30.0f;
    level_count++;

    if (max_levels >= 2) {
        int lod1_count = 0;
        Mesh lod1 = create_decimated_mesh(verts, nv, indices, ni, 2, &lod1_count);
        if (lod1.vao != 0 && lod1_count >= 3) {
            out_levels[level_count].mesh = lod1;
            out_levels[level_count].index_count = lod1_count;
            out_levels[level_count].max_distance = 80.0f;
            level_count++;
        }
    }

    if (max_levels >= 3) {
        int lod2_count = 0;
        Mesh lod2 = create_decimated_mesh(verts, nv, indices, ni, 4, &lod2_count);
        if (lod2.vao != 0 && lod2_count >= 3) {
            out_levels[level_count].mesh = lod2;
            out_levels[level_count].index_count = lod2_count;
            out_levels[level_count].max_distance = 1000000.0f;
            level_count++;
        }
    }

    if (level_count == 1) {
        out_levels[0].max_distance = 1000000.0f;
    }
    if (level_count == 2) {
        out_levels[1].max_distance = 1000000.0f;
    }
    return level_count;
}
