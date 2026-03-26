#ifndef ENGINE_TEXTURE_H
#define ENGINE_TEXTURE_H

#include "gl_funcs.h"
#include "render_settings.h"

GLuint texture_load_2d(const char *path, const RenderSettings *render_settings);
GLuint texture_create_flat_normal(const RenderSettings *render_settings);
void texture_destroy(GLuint tex);

#endif
