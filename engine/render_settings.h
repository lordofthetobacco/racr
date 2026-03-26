#ifndef ENGINE_RENDER_SETTINGS_H
#define ENGINE_RENDER_SETTINGS_H

#include "gl_funcs.h"
#include <stdbool.h>

typedef struct {
    bool anisotropy_supported;
    float anisotropy_target;
    float anisotropy_max;
} RenderSettings;

void render_settings_init(RenderSettings *rs);
void render_settings_detect_gl_caps(RenderSettings *rs);
void render_settings_apply_to_texture(const RenderSettings *rs, GLuint texture);

#endif
