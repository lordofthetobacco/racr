#include "render_settings.h"
#include <string.h>

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

static bool has_extension(const char *name) {
    GLint ext_count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);
    for (GLint i = 0; i < ext_count; i++) {
        const char *ext = (const char *)glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (ext && strcmp(ext, name) == 0) return true;
    }

    const char *fallback = (const char *)glGetString(GL_EXTENSIONS);
    if (!fallback) return false;
    return strstr(fallback, name) != NULL;
}

void render_settings_init(RenderSettings *rs) {
    rs->anisotropy_supported = false;
    rs->anisotropy_target = 1.0f;
    rs->anisotropy_max = 1.0f;
}

void render_settings_detect_gl_caps(RenderSettings *rs) {
    render_settings_init(rs);
    rs->anisotropy_supported = has_extension("GL_EXT_texture_filter_anisotropic");
    if (!rs->anisotropy_supported) return;

    GLfloat max_aniso = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
    if (max_aniso < 1.0f) max_aniso = 1.0f;
    rs->anisotropy_max = max_aniso;
    rs->anisotropy_target = max_aniso;
}

void render_settings_apply_to_texture(const RenderSettings *rs, GLuint texture) {
    if (!rs->anisotropy_supported || !texture) return;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, rs->anisotropy_target);
    glBindTexture(GL_TEXTURE_2D, 0);
}
