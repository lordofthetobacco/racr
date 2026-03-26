#ifndef ENGINE_NK_IMPL_H
#define ENGINE_NK_IMPL_H

#include <SDL3/SDL.h>

struct nk_context;

struct nk_context *nk_impl_init(SDL_Window *window);
void nk_impl_input_begin(void);
void nk_impl_input_end(void);
int nk_impl_handle_event(SDL_Event *ev);
void nk_impl_render(void);
void nk_impl_shutdown(void);

#endif
