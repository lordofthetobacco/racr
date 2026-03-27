#ifndef ENGINE_PHYSICS_H
#define ENGINE_PHYSICS_H

#include "math_types.h"
#include "mesh.h"
#include <stdbool.h>

typedef struct {
    vec3 min;
    vec3 max;
} AABB;

typedef struct {
    vec3 velocity;
    float mass;
    float restitution;
    float friction;
    bool enabled;
    bool grounded;
    AABB local_aabb;
} PhysicsBody;

struct SceneObject;

void physics_body_init(PhysicsBody *body);
AABB physics_compute_local_aabb(const Vertex *verts, int nv);
AABB physics_transform_aabb(const AABB *local, const mat4 *model);
bool physics_aabb_overlap(const AABB *a, const AABB *b);
void physics_step(struct SceneObject *objects, int count, float dt, float gravity);

#endif
