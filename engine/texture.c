#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint texture_load_2d(const char *path, const RenderSettings *render_settings) {
    if (!path || !path[0]) return 0;

    int w = 0, h = 0, channels = 0;
    unsigned char *pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels || w <= 0 || h <= 0) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (render_settings) render_settings_apply_to_texture(render_settings, tex);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);
    return tex;
}

GLuint texture_create_flat_normal(const RenderSettings *render_settings) {
    static const unsigned char flat_normal[4] = {128, 128, 255, 255};
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, flat_normal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (render_settings) render_settings_apply_to_texture(render_settings, tex);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void texture_destroy(GLuint tex) {
    if (tex) glDeleteTextures(1, &tex);
}
