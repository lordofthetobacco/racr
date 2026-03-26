#ifndef ENGINE_SHADER_H
#define ENGINE_SHADER_H

#include "gl_funcs.h"
#include "math_types.h"

GLuint shader_create(const char *vert_path, const char *frag_path);
void   shader_use(GLuint program);
void   shader_set_mat4(GLuint program, const char *name, const mat4 *m);
void   shader_set_vec3(GLuint program, const char *name, vec3 v);
void   shader_set_vec4(GLuint program, const char *name, vec4 v);
void   shader_set_float(GLuint program, const char *name, float v);
void   shader_set_int(GLuint program, const char *name, int v);
void   shader_destroy(GLuint program);

#endif
