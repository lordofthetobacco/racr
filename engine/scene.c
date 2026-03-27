#include "scene.h"
#include "texture.h"
#include <stdlib.h>
#include <string.h>

void scene_init(Scene *s) {
    s->objects  = NULL;
    s->count    = 0;
    s->capacity = 0;
    light_setup_init(&s->lights);
    render_settings_init(&s->render_settings);
}

void scene_add(Scene *s, Mesh mesh, const char *mesh_path,
               Transform transform,
               const Vertex *verts, int nv,
               const uint32_t *indices, int ni,
               const ObjGroup *groups, int group_count) {
    if (s->count >= s->capacity) {
        s->capacity = s->capacity ? s->capacity * 2 : 4;
        s->objects = realloc(s->objects, s->capacity * sizeof(SceneObject));
    }
    SceneObject *o = &s->objects[s->count++];
    o->mesh = mesh;
    o->lod_count = 0;
    memset(o->lods, 0, sizeof(o->lods));
    o->transform = transform;
    o->submeshes = NULL;
    o->sub_count = 0;
    if (mesh_path && mesh_path[0]) {
        strncpy(o->mesh_path, mesh_path, SCENE_MESH_PATH_MAX - 1);
        o->mesh_path[SCENE_MESH_PATH_MAX - 1] = '\0';
    } else {
        o->mesh_path[0] = '\0';
    }

    if (groups && group_count > 0) {
        o->sub_count = group_count;
        o->submeshes = calloc((size_t)o->sub_count, sizeof(SubMesh));
        for (int i = 0; i < o->sub_count; i++) {
            SubMesh *sm = &o->submeshes[i];
            strncpy(sm->name, groups[i].name, OBJ_GROUP_NAME_MAX - 1);
            sm->name[OBJ_GROUP_NAME_MAX - 1] = '\0';
            sm->first_index = groups[i].first_index;
            sm->index_count = groups[i].index_count;
            sm->material = material_default();
        }
    } else {
        o->sub_count = 1;
        o->submeshes = calloc(1, sizeof(SubMesh));
        strcpy(o->submeshes[0].name, "default");
        o->submeshes[0].first_index = 0;
        o->submeshes[0].index_count = mesh.index_count;
        o->submeshes[0].material = material_default();
    }

    physics_body_init(&o->physics);
    o->physics.local_aabb = physics_compute_local_aabb(verts, nv);

    if (verts && indices && nv > 0 && ni > 0) {
        o->lod_count = lod_generate_levels(verts, nv, indices, ni, o->lods, LOD_LEVEL_MAX);
        if (o->lod_count > 0) {
            mesh_destroy(&o->mesh);
            o->mesh = o->lods[0].mesh;
        }
    }
}

void scene_clear(Scene *s) {
    for (int i = 0; i < s->count; i++) {
        for (int sm = 0; sm < s->objects[i].sub_count; sm++) {
            texture_destroy(s->objects[i].submeshes[sm].material.normal_map_tex);
            s->objects[i].submeshes[sm].material.normal_map_tex = 0;
        }
        free(s->objects[i].submeshes);
        s->objects[i].submeshes = NULL;
        s->objects[i].sub_count = 0;
        if (s->objects[i].lod_count > 0) {
            lod_levels_destroy(s->objects[i].lods, s->objects[i].lod_count);
            s->objects[i].lod_count = 0;
        } else {
            mesh_destroy(&s->objects[i].mesh);
        }
    }
    s->count = 0;
}

void scene_destroy(Scene *s) {
    scene_clear(s);
    light_setup_destroy(&s->lights);
    free(s->objects);
    s->objects  = NULL;
    s->capacity = 0;
}

mat4 scene_object_model(const SceneObject *obj) {
    mat4 t = mat4_translate(obj->transform.position);
    mat4 rx = mat4_rotate_x(obj->transform.rotation.x);
    mat4 ry = mat4_rotate_y(obj->transform.rotation.y);
    mat4 rz = mat4_rotate_z(obj->transform.rotation.z);
    mat4 sc = mat4_scale(obj->transform.scale);
    return mat4_multiply(t, mat4_multiply(rz, mat4_multiply(ry, mat4_multiply(rx, sc))));
}

bool scene_set_submesh_material(SceneObject *obj, const char *submesh_name, Material material) {
    if (!obj || !submesh_name) return false;
    for (int i = 0; i < obj->sub_count; i++) {
        if (strcmp(obj->submeshes[i].name, submesh_name) == 0) {
            obj->submeshes[i].material = material;
            return true;
        }
    }
    return false;
}

void scene_set_submesh_material_index(SceneObject *obj, int submesh_index, Material material) {
    if (!obj || submesh_index < 0 || submesh_index >= obj->sub_count) return;
    obj->submeshes[submesh_index].material = material;
}
