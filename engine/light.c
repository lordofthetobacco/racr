#include "light.h"
#include <stdlib.h>

void light_setup_init(LightSetup *ls) {
    ls->directional.direction = vec3_normalize(vec3_new(0.5f, -1.0f, -0.3f));
    ls->directional.color = vec3_new(1.0f, 0.98f, 0.92f);
    ls->directional.intensity = 1.0f;
    ls->points = NULL;
    ls->point_count = 0;
    ls->point_capacity = 0;
}

void light_setup_reset_points(LightSetup *ls) {
    ls->point_count = 0;
}

void light_setup_destroy(LightSetup *ls) {
    free(ls->points);
    ls->points = NULL;
    ls->point_count = 0;
    ls->point_capacity = 0;
}

int light_setup_add_point(LightSetup *ls, PointLight p) {
    if (ls->point_count >= ls->point_capacity) {
        int new_cap = ls->point_capacity ? ls->point_capacity * 2 : 4;
        PointLight *new_points = realloc(ls->points, (size_t)new_cap * sizeof(PointLight));
        if (!new_points) return -1;
        ls->points = new_points;
        ls->point_capacity = new_cap;
    }
    p.direction = vec3_normalize(p.direction);
    if (vec3_length(p.direction) < 1e-4f) p.direction = vec3_new(0.0f, -1.0f, 0.0f);
    ls->points[ls->point_count] = p;
    ls->point_count++;
    return ls->point_count - 1;
}

void light_setup_remove_point(LightSetup *ls, int index) {
    if (index < 0 || index >= ls->point_count) return;
    for (int i = index; i + 1 < ls->point_count; i++) {
        ls->points[i] = ls->points[i + 1];
    }
    ls->point_count--;
}
