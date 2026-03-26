#include "pick.h"
#include <SDL3/SDL.h>
#include <string.h>

static bool pick_allocate(PickContext *p, int width, int height) {
    if (width <= 0 || height <= 0) return false;

    p->width = width;
    p->height = height;

    glGenFramebuffers(1, &p->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, p->fbo);

    glGenTextures(1, &p->color_tex);
    glBindTexture(GL_TEXTURE_2D, p->color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           p->color_tex, 0);

    glGenRenderbuffers(1, &p->depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, p->depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, p->depth_rbo);

    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!ok) {
        SDL_Log("pick: framebuffer incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    return ok;
}

bool pick_init(PickContext *p, int width, int height) {
    memset(p, 0, sizeof(*p));
    return pick_allocate(p, width, height);
}

bool pick_resize(PickContext *p, int width, int height) {
    if (p->width == width && p->height == height && p->fbo) return true;
    pick_destroy(p);
    return pick_allocate(p, width, height);
}

void pick_destroy(PickContext *p) {
    if (p->depth_rbo) glDeleteRenderbuffers(1, &p->depth_rbo);
    if (p->color_tex) glDeleteTextures(1, &p->color_tex);
    if (p->fbo) glDeleteFramebuffers(1, &p->fbo);
    memset(p, 0, sizeof(*p));
}

void pick_bind(const PickContext *p) {
    glBindFramebuffer(GL_FRAMEBUFFER, p->fbo);
    glViewport(0, 0, p->width, p->height);
}

void pick_unbind(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t pick_read_id_at(const PickContext *p, int mouse_x, int mouse_y) {
    if (!p->fbo) return 0;
    if (mouse_x < 0 || mouse_x >= p->width || mouse_y < 0 || mouse_y >= p->height)
        return 0;

    const int read_y = p->height - 1 - mouse_y;
    unsigned char rgb[3] = {0, 0, 0};
    glBindFramebuffer(GL_FRAMEBUFFER, p->fbo);
    glReadPixels(mouse_x, read_y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    return (uint32_t)rgb[0] | ((uint32_t)rgb[1] << 8) | ((uint32_t)rgb[2] << 16);
}

void pick_id_to_rgb(uint32_t id, float rgb[3]) {
    rgb[0] = ((id >> 0) & 0xFFu) / 255.0f;
    rgb[1] = ((id >> 8) & 0xFFu) / 255.0f;
    rgb[2] = ((id >> 16) & 0xFFu) / 255.0f;
}
