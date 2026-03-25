#ifndef ENGINE_CAMERA_H
#define ENGINE_CAMERA_H

#include "math_types.h"

typedef struct {
    vec3  position; /* eye in world space */
    float yaw;      /* radians, rotation around Y (same convention as old orbit) */
    float pitch;    /* radians, look up/down (clamped) */
    float fov;
    float aspect;
    float near_plane;
    float far_plane;
} Camera;

Camera camera_default(float aspect);
vec3   camera_forward(const Camera *c);
vec3   camera_right_flat(const Camera *c);
vec3   camera_position(const Camera *c);
mat4   camera_view(const Camera *c);
mat4   camera_projection(const Camera *c);
void   camera_apply_mouse_look(Camera *c, float dx, float dy, float sensitivity);
void   camera_move_fps(Camera *c, float forward, float right, float up, float step);
void   camera_pan(Camera *c, float right, float up);

#endif
