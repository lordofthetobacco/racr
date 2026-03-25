#ifndef ENGINE_SCENE_IO_H
#define ENGINE_SCENE_IO_H

#include "scene.h"
#include "camera.h"
#include <stdbool.h>

bool scene_save(const Scene *scene, const Camera *cam, const char *path);
bool scene_load(Scene *scene, Camera *cam, const char *path);

#endif
