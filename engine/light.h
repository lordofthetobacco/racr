#ifndef ENGINE_LIGHT_H
#define ENGINE_LIGHT_H

#include "math_types.h"

#define LIGHT_SHADER_POINT_MAX 8

typedef struct {
    vec3 direction;
    vec3 color;
    float intensity;
} DirectionalLight;

typedef struct {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
} PointLight;

typedef struct {
    DirectionalLight directional;
    PointLight *points;
    int point_count;
    int point_capacity;
} LightSetup;

void light_setup_init(LightSetup *ls);
void light_setup_reset_points(LightSetup *ls);
void light_setup_destroy(LightSetup *ls);
int light_setup_add_point(LightSetup *ls, PointLight p);
void light_setup_remove_point(LightSetup *ls, int index);

#endif
