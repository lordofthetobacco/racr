#include "camera.h"
#include <math.h>

/* World up for look-at; avoids parallel forward so cross(forward, up) != 0. */
static vec3 camera_world_up_for_forward(vec3 f) {
    vec3 u0 = vec3_new(0, 1, 0);
    if (fabsf(vec3_dot(f, u0)) < 0.85f)
        return u0;
    vec3 u1 = vec3_new(0, 0, 1);
    if (fabsf(vec3_dot(f, u1)) < 0.85f)
        return u1;
    return vec3_new(1, 0, 0);
}

/* Matches previous orbit offset direction: eye looks toward -offset. */
static vec3 forward_from_angles(float yaw, float pitch) {
    float cp = cosf(pitch), sp = sinf(pitch);
    float sy = sinf(yaw), cy = cosf(yaw);
    return vec3_normalize(vec3_new(-cp * sy, -sp, -cp * cy));
}

Camera camera_default(float aspect) {
    /* Same starting view as old orbit: target origin, dist 5, yaw 0, pitch 0.3 */
    float yaw   = 0.0f;
    float pitch = 0.3f;
    float d     = 5.0f;
    vec3 offset = vec3_new(d * cosf(pitch) * sinf(yaw), d * sinf(pitch),
                           d * cosf(pitch) * cosf(yaw));
    vec3 pos    = vec3_add(vec3_new(0, 0, 0), offset);

    return (Camera){
        .position   = pos,
        .yaw        = yaw,
        .pitch      = pitch,
        .fov        = 45.0f * (3.14159265f / 180.0f),
        .aspect     = aspect,
        .near_plane = 0.1f,
        .far_plane  = 100.0f,
    };
}

vec3 camera_forward(const Camera *c) {
    return forward_from_angles(c->yaw, c->pitch);
}

vec3 camera_right_flat(const Camera *c) {
    vec3 f = camera_forward(c);
    vec3 flat = vec3_new(f.x, 0.0f, f.z);
    if (vec3_length(flat) < 1e-6f)
        flat = vec3_new(-sinf(c->yaw), 0.0f, -cosf(c->yaw));
    else
        flat = vec3_normalize(flat);
    vec3 world_up = vec3_new(0, 1, 0);
    return vec3_normalize(vec3_cross(flat, world_up));
}

vec3 camera_position(const Camera *c) {
    return c->position;
}

mat4 camera_view(const Camera *c) {
    vec3 eye = c->position;
    vec3 f   = camera_forward(c);
    vec3 at  = vec3_add(eye, f);
    vec3 up  = camera_world_up_for_forward(f);
    return mat4_look_at(eye, at, up);
}

mat4 camera_projection(const Camera *c) {
    return mat4_perspective(c->fov, c->aspect, c->near_plane, c->far_plane);
}

void camera_apply_mouse_look(Camera *c, float dx, float dy, float sensitivity) {
    c->yaw -= dx * sensitivity;
    c->pitch -= dy * sensitivity;
    /* Stay below ~±85° so look-at never sees forward ∥ world Y (no roll twist). */
    const float lim = 1.48f;
    if (c->pitch > lim) c->pitch = lim;
    if (c->pitch < -lim) c->pitch = -lim;
    /* Keep yaw in [-pi, pi] for numerical stability. */
    const float twopi = 6.2831853f;
    const float pi    = 3.14159265f;
    while (c->yaw > pi) c->yaw -= twopi;
    while (c->yaw < -pi) c->yaw += twopi;
}

void camera_move_fps(Camera *c, float forward, float right, float up, float step) {
    vec3 f = camera_forward(c);
    vec3 flat_f = vec3_new(f.x, 0.0f, f.z);
    if (vec3_length(flat_f) > 1e-6f)
        flat_f = vec3_normalize(flat_f);
    vec3 r = camera_right_flat(c);

    c->position = vec3_add(c->position, vec3_scale(flat_f, forward * step));
    c->position = vec3_add(c->position, vec3_scale(r, right * step));
    c->position.y += up * step;
}

void camera_pan(Camera *c, float right, float up) {
    vec3 r = camera_right_flat(c);
    vec3 world_up = vec3_new(0, 1, 0);
    c->position = vec3_add(c->position, vec3_scale(r, right));
    c->position = vec3_add(c->position, vec3_scale(world_up, up));
}
