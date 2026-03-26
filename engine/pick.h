#ifndef ENGINE_PICK_H
#define ENGINE_PICK_H

#include "gl_funcs.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    GLuint fbo;
    GLuint color_tex;
    GLuint depth_rbo;
    int width;
    int height;
} PickContext;

bool pick_init(PickContext *p, int width, int height);
bool pick_resize(PickContext *p, int width, int height);
void pick_destroy(PickContext *p);
void pick_bind(const PickContext *p);
void pick_unbind(void);
uint32_t pick_read_id_at(const PickContext *p, int mouse_x, int mouse_y);
void pick_id_to_rgb(uint32_t id, float rgb[3]);

#endif
