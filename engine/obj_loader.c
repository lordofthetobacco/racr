#include "obj_loader.h"
#include "math_types.h"
#include <SDL3/SDL.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { float x, y, z; }  ObjVec3;
typedef struct { float u, v; }     ObjVec2;
typedef struct { int v, vt, vn; }  ObjIdx;

#define INIT_CAP 1024

#define DA_PUSH(arr, count, cap, val) do {                \
    if ((count) >= (cap)) {                               \
        (cap) = (cap) ? (cap) * 2 : INIT_CAP;            \
        (arr) = realloc((arr), (cap) * sizeof(*(arr)));  \
    }                                                     \
    (arr)[(count)++] = (val);                             \
} while (0)

static void compute_tangents(Vertex *verts, int nv, const uint32_t *indices, int ni) {
    if (!verts || !indices || nv <= 0 || ni < 3) return;

    float *tan1 = calloc((size_t)nv * 3u, sizeof(float));
    float *tan2 = calloc((size_t)nv * 3u, sizeof(float));
    if (!tan1 || !tan2) {
        free(tan1);
        free(tan2);
        return;
    }

    for (int i = 0; i + 2 < ni; i += 3) {
        int i0 = (int)indices[i];
        int i1 = (int)indices[i + 1];
        int i2 = (int)indices[i + 2];
        if (i0 < 0 || i0 >= nv || i1 < 0 || i1 >= nv || i2 < 0 || i2 >= nv) continue;

        const float *p0 = verts[i0].position;
        const float *p1 = verts[i1].position;
        const float *p2 = verts[i2].position;
        const float *uv0 = verts[i0].uv;
        const float *uv1 = verts[i1].uv;
        const float *uv2 = verts[i2].uv;

        float x1 = p1[0] - p0[0];
        float y1 = p1[1] - p0[1];
        float z1 = p1[2] - p0[2];
        float x2 = p2[0] - p0[0];
        float y2 = p2[1] - p0[1];
        float z2 = p2[2] - p0[2];

        float s1 = uv1[0] - uv0[0];
        float t1 = uv1[1] - uv0[1];
        float s2 = uv2[0] - uv0[0];
        float t2 = uv2[1] - uv0[1];

        float denom = (s1 * t2 - s2 * t1);
        float r = fabsf(denom) > 1e-8f ? (1.0f / denom) : 1.0f;

        float tx = (t2 * x1 - t1 * x2) * r;
        float ty = (t2 * y1 - t1 * y2) * r;
        float tz = (t2 * z1 - t1 * z2) * r;
        float bx = (s1 * x2 - s2 * x1) * r;
        float by = (s1 * y2 - s2 * y1) * r;
        float bz = (s1 * z2 - s2 * z1) * r;

        int idx[3] = {i0, i1, i2};
        for (int k = 0; k < 3; k++) {
            int v = idx[k];
            tan1[v * 3 + 0] += tx;
            tan1[v * 3 + 1] += ty;
            tan1[v * 3 + 2] += tz;
            tan2[v * 3 + 0] += bx;
            tan2[v * 3 + 1] += by;
            tan2[v * 3 + 2] += bz;
        }
    }

    for (int i = 0; i < nv; i++) {
        vec3 n = vec3_new(verts[i].normal[0], verts[i].normal[1], verts[i].normal[2]);
        vec3 t = vec3_new(tan1[i * 3 + 0], tan1[i * 3 + 1], tan1[i * 3 + 2]);
        t = vec3_sub(t, vec3_scale(n, vec3_dot(n, t)));
        t = vec3_normalize(t);
        if (vec3_length(t) < 1e-6f) t = vec3_new(1.0f, 0.0f, 0.0f);

        vec3 b = vec3_new(tan2[i * 3 + 0], tan2[i * 3 + 1], tan2[i * 3 + 2]);
        vec3 c = vec3_cross(n, t);
        float handedness = (vec3_dot(c, b) < 0.0f) ? -1.0f : 1.0f;

        verts[i].tangent[0] = t.x;
        verts[i].tangent[1] = t.y;
        verts[i].tangent[2] = t.z;
        verts[i].tangent[3] = handedness;
    }

    free(tan1);
    free(tan2);
}

static void copy_group_name(char dst[OBJ_GROUP_NAME_MAX], const char *src) {
    if (!src || !src[0]) {
        strcpy(dst, "default");
        return;
    }
    strncpy(dst, src, OBJ_GROUP_NAME_MAX - 1);
    dst[OBJ_GROUP_NAME_MAX - 1] = '\0';
}

static int parse_face_idx(const char **p, int nv, int nvt, int nvn,
                          int *vi, int *vti, int *vni) {
    char *end;
    long v = strtol(*p, &end, 10);
    if (end == *p) return 0;
    *p = end;
    *vi  = (int)(v > 0 ? v - 1 : nv + (int)v);
    *vti = -1;
    *vni = -1;

    if (**p == '/') {
        (*p)++;
        if (**p != '/') {
            long t = strtol(*p, &end, 10);
            *vti = (int)(t > 0 ? t - 1 : nvt + (int)t);
            *p = end;
        }
        if (**p == '/') {
            (*p)++;
            long n = strtol(*p, &end, 10);
            *vni = (int)(n > 0 ? n - 1 : nvn + (int)n);
            *p = end;
        }
    }
    return 1;
}

static void begin_group(ObjGroup **groups, int *count, int *cap, int ni,
                        const char *name, int *current_group) {
    if (*count == 0) {
        ObjGroup g;
        copy_group_name(g.name, name);
        g.first_index = ni;
        g.index_count = 0;
        DA_PUSH(*groups, *count, *cap, g);
        *current_group = 0;
        return;
    }

    ObjGroup *cur = &(*groups)[*current_group];
    if (cur->index_count == 0 && cur->first_index == ni) {
        copy_group_name(cur->name, name);
        return;
    }

    ObjGroup g;
    copy_group_name(g.name, name);
    g.first_index = ni;
    g.index_count = 0;
    DA_PUSH(*groups, *count, *cap, g);
    *current_group = *count - 1;
}

bool obj_load_grouped(const char *path,
                      Vertex **out_verts, int *out_nv,
                      uint32_t **out_indices, int *out_ni,
                      ObjGroup **out_groups, int *out_group_count) {
    FILE *f = fopen(path, "r");
    if (!f) {
        SDL_Log("Cannot open OBJ: %s", path);
        return false;
    }

    ObjVec3 *positions = NULL; int np = 0, cp = 0;
    ObjVec3 *normals   = NULL; int nn = 0, cn = 0;
    ObjVec2 *texcoords = NULL; int nt = 0, ct = 0;

    Vertex   *verts   = NULL; int nv = 0, cv = 0;
    uint32_t *indices = NULL; int ni = 0, ci = 0;

    ObjGroup *groups = NULL; int ng = 0, cg = 0, gcap = 0;
    begin_group(&groups, &ng, &gcap, 0, "default", &cg);

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if ((line[0] == 'o' || line[0] == 'g') && isspace((unsigned char)line[1])) {
            const char *p = line + 2;
            while (*p && isspace((unsigned char)*p)) p++;
            char gname[OBJ_GROUP_NAME_MAX];
            int i = 0;
            while (*p && *p != '\n' && *p != '\r' && !isspace((unsigned char)*p) &&
                   i < OBJ_GROUP_NAME_MAX - 1) {
                gname[i++] = *p++;
            }
            gname[i] = '\0';
            begin_group(&groups, &ng, &gcap, ni, gname, &cg);
            continue;
        }

        if (line[0] == 'v' && line[1] == ' ') {
            ObjVec3 p;
            sscanf(line + 2, "%f %f %f", &p.x, &p.y, &p.z);
            DA_PUSH(positions, np, cp, p);
        } else if (line[0] == 'v' && line[1] == 'n') {
            ObjVec3 n;
            sscanf(line + 3, "%f %f %f", &n.x, &n.y, &n.z);
            DA_PUSH(normals, nn, cn, n);
        } else if (line[0] == 'v' && line[1] == 't') {
            ObjVec2 t;
            sscanf(line + 3, "%f %f", &t.u, &t.v);
            DA_PUSH(texcoords, nt, ct, t);
        } else if (line[0] == 'f' && line[1] == ' ') {
            const char *p = line + 2;
            ObjIdx face[16];
            int fc = 0;

            while (*p && *p != '\n' && *p != '\r') {
                while (*p == ' ') p++;
                if (!*p || *p == '\n' || *p == '\r') break;
                ObjIdx idx;
                if (!parse_face_idx(&p, np, nt, nn, &idx.v, &idx.vt, &idx.vn))
                    break;
                if (fc < 16) face[fc++] = idx;
            }

            for (int i = 0; i < fc; i++) {
                Vertex vert = {{0}};
                if (face[i].v >= 0 && face[i].v < np) {
                    vert.position[0] = positions[face[i].v].x;
                    vert.position[1] = positions[face[i].v].y;
                    vert.position[2] = positions[face[i].v].z;
                }
                if (face[i].vn >= 0 && face[i].vn < nn) {
                    vert.normal[0] = normals[face[i].vn].x;
                    vert.normal[1] = normals[face[i].vn].y;
                    vert.normal[2] = normals[face[i].vn].z;
                }
                if (face[i].vt >= 0 && face[i].vt < nt) {
                    vert.uv[0] = texcoords[face[i].vt].u;
                    vert.uv[1] = texcoords[face[i].vt].v;
                }
                vert.tangent[0] = 1.0f;
                vert.tangent[1] = 0.0f;
                vert.tangent[2] = 0.0f;
                vert.tangent[3] = 1.0f;
                DA_PUSH(verts, nv, cv, vert);
            }

            /* triangulate fan: 0-1-2, 0-2-3, 0-3-4, ... */
            int base = nv - fc;
            for (int i = 1; i + 1 < fc; i++) {
                DA_PUSH(indices, ni, ci, (uint32_t)base);
                DA_PUSH(indices, ni, ci, (uint32_t)(base + i));
                DA_PUSH(indices, ni, ci, (uint32_t)(base + i + 1));
                groups[cg].index_count += 3;
            }
        }
    }
    fclose(f);

    if (nn == 0 && ni >= 3) {
        for (int i = 0; i + 2 < ni; i += 3) {
            float *p0 = verts[indices[i]].position;
            float *p1 = verts[indices[i + 1]].position;
            float *p2 = verts[indices[i + 2]].position;
            float ax = p1[0] - p0[0], ay = p1[1] - p0[1], az = p1[2] - p0[2];
            float bx = p2[0] - p0[0], by = p2[1] - p0[1], bz = p2[2] - p0[2];
            float nx = ay * bz - az * by;
            float ny = az * bx - ax * bz;
            float nz = ax * by - ay * bx;
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 1e-8f) {
                nx /= len;
                ny /= len;
                nz /= len;
            }
            for (int j = 0; j < 3; j++) {
                verts[indices[i + j]].normal[0] = nx;
                verts[indices[i + j]].normal[1] = ny;
                verts[indices[i + j]].normal[2] = nz;
            }
        }
    }

    compute_tangents(verts, nv, indices, ni);

    free(positions);
    free(normals);
    free(texcoords);

    if (ni == 0) {
        free(verts);
        free(indices);
        free(groups);
        SDL_Log("OBJ has no faces: %s", path);
        return false;
    }

    int wr = 0;
    for (int i = 0; i < ng; i++) {
        if (groups[i].index_count <= 0) continue;
        groups[wr++] = groups[i];
    }
    ng = wr;
    if (ng == 0) {
        ng = 1;
        groups = realloc(groups, sizeof(ObjGroup));
        copy_group_name(groups[0].name, "default");
        groups[0].first_index = 0;
        groups[0].index_count = ni;
    }

    SDL_Log("Loaded OBJ: %s (%d verts, %d indices, %d groups)", path, nv, ni, ng);
    *out_verts = verts;
    *out_nv = nv;
    *out_indices = indices;
    *out_ni = ni;
    if (out_groups && out_group_count) {
        *out_groups = groups;
        *out_group_count = ng;
    } else {
        free(groups);
    }
    return true;
}

bool obj_load(const char *path,
              Vertex **out_verts, int *out_nv,
              uint32_t **out_indices, int *out_ni) {
    ObjGroup *groups = NULL;
    int group_count = 0;
    bool ok = obj_load_grouped(path, out_verts, out_nv, out_indices, out_ni,
                               &groups, &group_count);
    free(groups);
    return ok;
}
