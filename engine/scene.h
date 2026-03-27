#ifndef ENGINE_SCENE_H
#define ENGINE_SCENE_H

#include "light.h"
#include "lod.h"
#include "material.h"
#include "mesh.h"
#include "math_types.h"
#include "obj_loader.h"
#include "physics.h"
#include "render_settings.h"
#include <stdbool.h>

#define SCENE_MESH_PATH_MAX 512

typedef struct {
    vec3 position;
    vec3 rotation;
    vec3 scale;
} Transform;

typedef struct {
    char name[OBJ_GROUP_NAME_MAX];
    int first_index;
    int index_count;
    Material material;
} SubMesh;

typedef struct SceneObject {
    Mesh mesh;
    LODLevel lods[LOD_LEVEL_MAX];
    int lod_count;
    char mesh_path[SCENE_MESH_PATH_MAX];
    Transform transform;
    SubMesh *submeshes;
    int sub_count;
    PhysicsBody physics;
} SceneObject;

typedef struct {
    SceneObject *objects;
    int count;
    int capacity;
    LightSetup lights;
    RenderSettings render_settings;
} Scene;

void scene_init(Scene *s);
void scene_add(Scene *s, Mesh mesh, const char *mesh_path,
               Transform transform,
               const Vertex *verts, int nv,
               const uint32_t *indices, int ni,
               const ObjGroup *groups, int group_count);
void scene_clear(Scene *s);
void scene_destroy(Scene *s);
bool scene_set_submesh_material(SceneObject *obj, const char *submesh_name, Material material);
void scene_set_submesh_material_index(SceneObject *obj, int submesh_index, Material material);

mat4 scene_object_model(const SceneObject *obj);

#endif
