#include "physics.h"
#include "scene.h"

#include <float.h>
#include <math.h>

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static vec3 aabb_center(const AABB *aabb) {
    return vec3_scale(vec3_add(aabb->min, aabb->max), 0.5f);
}

static vec3 transform_point(const mat4 *m, vec3 p) {
    return vec3_new(
        m->m[0] * p.x + m->m[4] * p.y + m->m[8] * p.z + m->m[12],
        m->m[1] * p.x + m->m[5] * p.y + m->m[9] * p.z + m->m[13],
        m->m[2] * p.x + m->m[6] * p.y + m->m[10] * p.z + m->m[14]
    );
}

static AABB world_aabb_for_object(const SceneObject *obj) {
    mat4 model = scene_object_model(obj);
    return physics_transform_aabb(&obj->physics.local_aabb, &model);
}

void physics_body_init(PhysicsBody *body) {
    if (!body) return;
    body->velocity = vec3_new(0.0f, 0.0f, 0.0f);
    body->mass = 1.0f;
    body->restitution = 0.3f;
    body->friction = 0.5f;
    body->enabled = true;
    body->grounded = false;
    body->local_aabb.min = vec3_new(-0.5f, -0.5f, -0.5f);
    body->local_aabb.max = vec3_new(0.5f, 0.5f, 0.5f);
}

AABB physics_compute_local_aabb(const Vertex *verts, int nv) {
    if (!verts || nv <= 0) {
        return (AABB){
            .min = vec3_new(-0.5f, -0.5f, -0.5f),
            .max = vec3_new(0.5f, 0.5f, 0.5f),
        };
    }

    vec3 min_v = vec3_new(FLT_MAX, FLT_MAX, FLT_MAX);
    vec3 max_v = vec3_new(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int i = 0; i < nv; i++) {
        vec3 p = vec3_new(verts[i].position[0], verts[i].position[1], verts[i].position[2]);
        if (p.x < min_v.x) min_v.x = p.x;
        if (p.y < min_v.y) min_v.y = p.y;
        if (p.z < min_v.z) min_v.z = p.z;
        if (p.x > max_v.x) max_v.x = p.x;
        if (p.y > max_v.y) max_v.y = p.y;
        if (p.z > max_v.z) max_v.z = p.z;
    }
    return (AABB){.min = min_v, .max = max_v};
}

AABB physics_transform_aabb(const AABB *local, const mat4 *model) {
    if (!local || !model) {
        return (AABB){0};
    }

    vec3 corners[8] = {
        vec3_new(local->min.x, local->min.y, local->min.z),
        vec3_new(local->max.x, local->min.y, local->min.z),
        vec3_new(local->min.x, local->max.y, local->min.z),
        vec3_new(local->max.x, local->max.y, local->min.z),
        vec3_new(local->min.x, local->min.y, local->max.z),
        vec3_new(local->max.x, local->min.y, local->max.z),
        vec3_new(local->min.x, local->max.y, local->max.z),
        vec3_new(local->max.x, local->max.y, local->max.z),
    };

    vec3 min_v = vec3_new(FLT_MAX, FLT_MAX, FLT_MAX);
    vec3 max_v = vec3_new(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int i = 0; i < 8; i++) {
        vec3 p = transform_point(model, corners[i]);
        if (p.x < min_v.x) min_v.x = p.x;
        if (p.y < min_v.y) min_v.y = p.y;
        if (p.z < min_v.z) min_v.z = p.z;
        if (p.x > max_v.x) max_v.x = p.x;
        if (p.y > max_v.y) max_v.y = p.y;
        if (p.z > max_v.z) max_v.z = p.z;
    }
    return (AABB){.min = min_v, .max = max_v};
}

bool physics_aabb_overlap(const AABB *a, const AABB *b) {
    if (!a || !b) return false;
    if (a->max.x < b->min.x || a->min.x > b->max.x) return false;
    if (a->max.y < b->min.y || a->min.y > b->max.y) return false;
    if (a->max.z < b->min.z || a->min.z > b->max.z) return false;
    return true;
}

void physics_step(SceneObject *objects, int count, float dt, float gravity) {
    if (!objects || count <= 0 || dt <= 0.0f) return;

    const float max_substep = 1.0f / 120.0f;
    float remaining = dt;
    while (remaining > 0.0f) {
        float step_dt = (remaining > max_substep) ? max_substep : remaining;
        remaining -= step_dt;

        for (int i = 0; i < count; i++) {
            SceneObject *obj = &objects[i];
            PhysicsBody *body = &obj->physics;
            body->grounded = false;
            if (!body->enabled) continue;

            body->velocity.y -= gravity * step_dt;
            obj->transform.position = vec3_add(obj->transform.position, vec3_scale(body->velocity, step_dt));
        }

        for (int i = 0; i < count; i++) {
            SceneObject *obj = &objects[i];
            PhysicsBody *body = &obj->physics;
            if (!body->enabled) continue;

            AABB world = world_aabb_for_object(obj);
            if (world.min.y < 0.0f) {
                obj->transform.position.y -= world.min.y;
                if (body->velocity.y < 0.0f) {
                    body->velocity.y = -body->velocity.y * clampf(body->restitution, 0.0f, 1.0f);
                }
                float friction = clampf(body->friction, 0.0f, 3.0f);
                float damp = fmaxf(0.0f, 1.0f - friction * step_dt * 5.0f);
                body->velocity.x *= damp;
                body->velocity.z *= damp;
                body->grounded = true;
            }
        }

        for (int i = 0; i < count; i++) {
            SceneObject *a = &objects[i];
            if (!a->physics.enabled) continue;
            for (int j = i + 1; j < count; j++) {
                SceneObject *b = &objects[j];
                if (!b->physics.enabled) continue;

                AABB aabb_a = world_aabb_for_object(a);
                AABB aabb_b = world_aabb_for_object(b);
                if (!physics_aabb_overlap(&aabb_a, &aabb_b)) continue;

                float overlap_x = fminf(aabb_a.max.x, aabb_b.max.x) - fmaxf(aabb_a.min.x, aabb_b.min.x);
                float overlap_y = fminf(aabb_a.max.y, aabb_b.max.y) - fmaxf(aabb_a.min.y, aabb_b.min.y);
                float overlap_z = fminf(aabb_a.max.z, aabb_b.max.z) - fmaxf(aabb_a.min.z, aabb_b.min.z);
                if (overlap_x <= 0.0f || overlap_y <= 0.0f || overlap_z <= 0.0f) continue;

                vec3 center_a = aabb_center(&aabb_a);
                vec3 center_b = aabb_center(&aabb_b);
                vec3 normal = vec3_new(1.0f, 0.0f, 0.0f);
                float penetration = overlap_x;

                if (overlap_y < penetration) {
                    penetration = overlap_y;
                    normal = vec3_new(0.0f, 1.0f, 0.0f);
                }
                if (overlap_z < penetration) {
                    penetration = overlap_z;
                    normal = vec3_new(0.0f, 0.0f, 1.0f);
                }

                if (normal.x != 0.0f && center_b.x < center_a.x) normal.x = -1.0f;
                if (normal.y != 0.0f && center_b.y < center_a.y) normal.y = -1.0f;
                if (normal.z != 0.0f && center_b.z < center_a.z) normal.z = -1.0f;

                float inv_mass_a = (a->physics.mass > 1e-5f) ? (1.0f / a->physics.mass) : 0.0f;
                float inv_mass_b = (b->physics.mass > 1e-5f) ? (1.0f / b->physics.mass) : 0.0f;
                float inv_mass_sum = inv_mass_a + inv_mass_b;
                if (inv_mass_sum <= 0.0f) continue;

                vec3 correction = vec3_scale(normal, penetration / inv_mass_sum);
                a->transform.position = vec3_sub(a->transform.position, vec3_scale(correction, inv_mass_a));
                b->transform.position = vec3_add(b->transform.position, vec3_scale(correction, inv_mass_b));

                vec3 rel_v = vec3_sub(b->physics.velocity, a->physics.velocity);
                float rel_n = vec3_dot(rel_v, normal);
                if (rel_n < 0.0f) {
                    float restitution = clampf((a->physics.restitution + b->physics.restitution) * 0.5f, 0.0f, 1.0f);
                    float j = -(1.0f + restitution) * rel_n / inv_mass_sum;
                    vec3 impulse = vec3_scale(normal, j);
                    a->physics.velocity = vec3_sub(a->physics.velocity, vec3_scale(impulse, inv_mass_a));
                    b->physics.velocity = vec3_add(b->physics.velocity, vec3_scale(impulse, inv_mass_b));

                    vec3 rel_after = vec3_sub(b->physics.velocity, a->physics.velocity);
                    vec3 tangent = vec3_sub(rel_after, vec3_scale(normal, vec3_dot(rel_after, normal)));
                    float tangent_len = vec3_length(tangent);
                    if (tangent_len > 1e-6f) {
                        tangent = vec3_scale(tangent, 1.0f / tangent_len);
                        float jt = -vec3_dot(rel_after, tangent) / inv_mass_sum;
                        float mu = clampf((a->physics.friction + b->physics.friction) * 0.5f, 0.0f, 3.0f);
                        float jt_limit = j * mu;
                        jt = clampf(jt, -jt_limit, jt_limit);
                        vec3 friction_impulse = vec3_scale(tangent, jt);
                        a->physics.velocity = vec3_sub(a->physics.velocity, vec3_scale(friction_impulse, inv_mass_a));
                        b->physics.velocity = vec3_add(b->physics.velocity, vec3_scale(friction_impulse, inv_mass_b));
                    }
                }
            }
        }
    }
}
