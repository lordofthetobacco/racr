#include "scene.h"
#include <string.h>
#include <stdlib.h>

void scene_init(Scene *s) {
    s->objects  = NULL;
    s->count    = 0;
    s->capacity = 0;
}

void scene_add(Scene *s, Mesh mesh, const char *mesh_path,
                vec3 pos, float rot_y, float scl) {
    if (s->count >= s->capacity) {
        s->capacity = s->capacity ? s->capacity * 2 : 4;
        s->objects = realloc(s->objects, s->capacity * sizeof(SceneObject));
    }
    SceneObject *o = &s->objects[s->count++];
    o->mesh       = mesh;
    o->position   = pos;
    o->rotation_y = rot_y;
    o->scale      = scl;
    if (mesh_path && mesh_path[0]) {
        strncpy(o->mesh_path, mesh_path, SCENE_MESH_PATH_MAX - 1);
        o->mesh_path[SCENE_MESH_PATH_MAX - 1] = '\0';
    } else {
        o->mesh_path[0] = '\0';
    }
}

void scene_clear(Scene *s) {
    for (int i = 0; i < s->count; i++)
        mesh_destroy(&s->objects[i].mesh);
    s->count = 0;
}

void scene_destroy(Scene *s) {
    scene_clear(s);
    free(s->objects);
    s->objects  = NULL;
    s->capacity = 0;
}

mat4 scene_object_model(const SceneObject *obj) {
    mat4 t = mat4_translate(obj->position);
    mat4 r = mat4_rotate_y(obj->rotation_y);
    mat4 sc = mat4_scale(vec3_new(obj->scale, obj->scale, obj->scale));
    return mat4_multiply(t, mat4_multiply(r, sc));
}
