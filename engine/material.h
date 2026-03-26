#ifndef ENGINE_MATERIAL_H
#define ENGINE_MATERIAL_H

#include "math_types.h"
#include "gl_funcs.h"

#define MATERIAL_NAME_MAX 64

typedef struct {
    vec4 base_color;
    float metallic;
    float roughness;
    char normal_map_path[256];
    GLuint normal_map_tex;
    char name[MATERIAL_NAME_MAX];
} Material;

static inline Material material_default(void) {
    Material m;
    m.base_color = vec4_new(0.75f, 0.75f, 0.75f, 1.0f);
    m.metallic = 0.0f;
    m.roughness = 0.9f;
    m.normal_map_path[0] = '\0';
    m.normal_map_tex = 0;
    m.name[0] = '\0';
    return m;
}

#endif
