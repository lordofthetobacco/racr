#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned char *stbi_load(const char *filename, int *x, int *y, int *channels_in_file,
                         int desired_channels);
void stbi_image_free(void *retval_from_stbi_load);

#ifdef STB_IMAGE_IMPLEMENTATION
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

unsigned char *stbi_load(const char *filename, int *x, int *y, int *channels_in_file,
                         int desired_channels) {
    (void)desired_channels;
    SDL_Surface *surface = SDL_LoadBMP(filename);
    if (!surface) return NULL;

    SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (!rgba) return NULL;

    int width = rgba->w;
    int height = rgba->h;
    size_t size = (size_t)width * (size_t)height * 4u;
    unsigned char *pixels = malloc(size);
    if (!pixels) {
        SDL_DestroySurface(rgba);
        return NULL;
    }

    memcpy(pixels, rgba->pixels, size);
    SDL_DestroySurface(rgba);
    if (x) *x = width;
    if (y) *y = height;
    if (channels_in_file) *channels_in_file = 4;
    return pixels;
}

void stbi_image_free(void *retval_from_stbi_load) {
    free(retval_from_stbi_load);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
