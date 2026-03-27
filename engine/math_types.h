#ifndef ENGINE_MATH_TYPES_H
#define ENGINE_MATH_TYPES_H

#include <math.h>
#include <string.h>

typedef struct { float x, y, z; } vec3;
typedef struct { float x, y, z, w; } vec4;
typedef struct { float m[16]; } mat4; /* column-major */

static inline vec3 vec3_new(float x, float y, float z) {
    return (vec3){x, y, z};
}

static inline vec4 vec4_new(float x, float y, float z, float w) {
    return (vec4){x, y, z, w};
}

static inline vec3 vec3_add(vec3 a, vec3 b) {
    return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline vec3 vec3_sub(vec3 a, vec3 b) {
    return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline vec3 vec3_scale(vec3 v, float s) {
    return (vec3){v.x * s, v.y * s, v.z * s};
}

static inline float vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3 vec3_cross(vec3 a, vec3 b) {
    return (vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float vec3_length(vec3 v) {
    return sqrtf(vec3_dot(v, v));
}

static inline vec3 vec3_normalize(vec3 v) {
    float len = vec3_length(v);
    if (len < 1e-8f) return (vec3){0, 0, 0};
    return vec3_scale(v, 1.0f / len);
}

static inline mat4 mat4_identity(void) {
    mat4 r;
    memset(&r, 0, sizeof(r));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static inline mat4 mat4_multiply(mat4 a, mat4 b) {
    mat4 r;
    for (int c = 0; c < 4; c++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
                sum += a.m[k * 4 + row] * b.m[c * 4 + k];
            r.m[c * 4 + row] = sum;
        }
    }
    return r;
}

static inline mat4 mat4_translate(vec3 t) {
    mat4 r = mat4_identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

static inline mat4 mat4_scale(vec3 s) {
    mat4 r = mat4_identity();
    r.m[0]  = s.x;
    r.m[5]  = s.y;
    r.m[10] = s.z;
    return r;
}

static inline mat4 mat4_rotate_y(float rad) {
    mat4 r = mat4_identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[0]  =  c;
    r.m[2]  = -s;
    r.m[8]  =  s;
    r.m[10] =  c;
    return r;
}

static inline mat4 mat4_rotate_x(float rad) {
    mat4 r = mat4_identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[5]  =  c;
    r.m[6]  =  s;
    r.m[9]  = -s;
    r.m[10] =  c;
    return r;
}

static inline mat4 mat4_rotate_z(float rad) {
    mat4 r = mat4_identity();
    float c = cosf(rad), s = sinf(rad);
    r.m[0] = c;
    r.m[1] = s;
    r.m[4] = -s;
    r.m[5] = c;
    return r;
}

static inline mat4 mat4_perspective(float fov_rad, float aspect,
                                     float near, float far) {
    mat4 r;
    memset(&r, 0, sizeof(r));
    float f = 1.0f / tanf(fov_rad * 0.5f);
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
}

static inline mat4 mat4_ortho(float left, float right, float bottom, float top,
                              float near, float far) {
    mat4 r = mat4_identity();
    r.m[0] = 2.0f / (right - left);
    r.m[5] = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (far - near);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(far + near) / (far - near);
    return r;
}

static inline mat4 mat4_look_at(vec3 eye, vec3 center, vec3 up) {
    vec3 f = vec3_normalize(vec3_sub(center, eye));
    vec3 s = vec3_normalize(vec3_cross(f, up));
    vec3 u = vec3_cross(s, f);

    mat4 r = mat4_identity();
    r.m[0]  =  s.x;
    r.m[4]  =  s.y;
    r.m[8]  =  s.z;
    r.m[1]  =  u.x;
    r.m[5]  =  u.y;
    r.m[9]  =  u.z;
    r.m[2]  = -f.x;
    r.m[6]  = -f.y;
    r.m[10] = -f.z;
    r.m[12] = -vec3_dot(s, eye);
    r.m[13] = -vec3_dot(u, eye);
    r.m[14] =  vec3_dot(f, eye);
    return r;
}

#endif /* ENGINE_MATH_TYPES_H */
