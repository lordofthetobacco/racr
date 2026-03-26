#include "nk_impl.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_BOOL
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define NK_SDL3_GL3_IMPLEMENTATION
#include "nuklear_sdl3_gl3.h"

static struct nk_context *g_ctx = NULL;

struct nk_context *nk_impl_init(SDL_Window *window) {
    struct nk_font_atlas *atlas = NULL;
    g_ctx = nk_sdl_init(window);
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();
    return g_ctx;
}

void nk_impl_input_begin(void) {
    nk_input_begin(g_ctx);
}

void nk_impl_input_end(void) {
    nk_input_end(g_ctx);
}

int nk_impl_handle_event(SDL_Event *ev) {
    return nk_sdl_handle_event(ev);
}

void nk_impl_render(void) {
    nk_sdl_render(NK_ANTI_ALIASING_ON, 512 * 1024, 128 * 1024);
}

void nk_impl_shutdown(void) {
    nk_sdl_shutdown();
    g_ctx = NULL;
}
