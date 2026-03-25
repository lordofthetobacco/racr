#ifndef ENGINE_SCENE_H
#define ENGINE_SCENE_H

#include "mesh.h"
#include "math_types.h"

#define SCENE_MESH_PATH_MAX 512

typedef struct {
    Mesh  mesh;
    char  mesh_path[SCENE_MESH_PATH_MAX];
    vec3  position;
    float rotation_y;
    float scale;
} SceneObject;

typedef struct {
    SceneObject *objects;
    int count;
    int capacity;
} Scene;

void scene_init(Scene *s);
void scene_add(Scene *s, Mesh mesh, const char *mesh_path,
                vec3 pos, float rot_y, float scale);
void scene_clear(Scene *s);
void scene_destroy(Scene *s);

mat4 scene_object_model(const SceneObject *obj);

#endif
