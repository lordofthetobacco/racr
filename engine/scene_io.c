#include "scene_io.h"
#include "obj_loader.h"
#include "mesh.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool scene_save(const Scene *scene, const Camera *cam, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) {
        SDL_Log("scene_save: cannot open %s", path);
        return false;
    }

    fprintf(f, "racr_scene 1\n");
    fprintf(f, "camera %g %g %g %g %g %g %g %g %g\n",
            (double)cam->position.x, (double)cam->position.y,
            (double)cam->position.z, (double)cam->yaw, (double)cam->pitch,
            (double)cam->fov, (double)cam->aspect, (double)cam->near_plane,
            (double)cam->far_plane);
    fprintf(f, "render %g\n", (double)scene->render_settings.anisotropy_target);
    fprintf(f, "directional_light %g %g %g %g %g %g %g\n",
            (double)scene->lights.directional.direction.x,
            (double)scene->lights.directional.direction.y,
            (double)scene->lights.directional.direction.z,
            (double)scene->lights.directional.color.x,
            (double)scene->lights.directional.color.y,
            (double)scene->lights.directional.color.z,
            (double)scene->lights.directional.intensity);
    for (int i = 0; i < scene->lights.point_count; i++) {
        const PointLight *pl = &scene->lights.points[i];
        fprintf(f, "point_light %g %g %g %g %g %g %g %g %g %g %g\n",
                (double)pl->position.x, (double)pl->position.y, (double)pl->position.z,
                (double)pl->direction.x, (double)pl->direction.y, (double)pl->direction.z,
                (double)pl->color.x, (double)pl->color.y, (double)pl->color.z,
                (double)pl->intensity, (double)pl->range);
    }

    for (int i = 0; i < scene->count; i++) {
        const SceneObject *o = &scene->objects[i];
        const char *mp = o->mesh_path[0] ? o->mesh_path : "assets/model.obj";
        fprintf(f, "object %s %g %g %g %g %g %g %g %g %g\n", mp,
                (double)o->transform.position.x,
                (double)o->transform.position.y,
                (double)o->transform.position.z,
                (double)o->transform.rotation.x,
                (double)o->transform.rotation.y,
                (double)o->transform.rotation.z,
                (double)o->transform.scale.x,
                (double)o->transform.scale.y,
                (double)o->transform.scale.z);
        for (int s = 0; s < o->sub_count; s++) {
            const SubMesh *sm = &o->submeshes[s];
            fprintf(f, "submaterial %s %g %g %g %g %g %g %s\n", sm->name,
                    (double)sm->material.base_color.x,
                    (double)sm->material.base_color.y,
                    (double)sm->material.base_color.z,
                    (double)sm->material.base_color.w,
                    (double)sm->material.metallic,
                    (double)sm->material.roughness,
                    sm->material.normal_map_path[0] ? sm->material.normal_map_path : "-");
        }
        fprintf(f, "physics %g %g %g %d\n",
                (double)o->physics.mass,
                (double)o->physics.restitution,
                (double)o->physics.friction,
                o->physics.enabled ? 1 : 0);
    }

    fclose(f);
    SDL_Log("Scene saved: %s (%d objects)", path, scene->count);
    return true;
}

bool scene_load(Scene *scene, Camera *cam, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        SDL_Log("scene_load: cannot open %s", path);
        return false;
    }

    char line[768];
    int header_ok = 0;

    SceneObject *current_obj = NULL;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        if (strncmp(line, "racr_scene ", 11) == 0) {
            int ver = 0;
            if (sscanf(line + 11, "%d", &ver) == 1 && ver == 1)
                header_ok = 1;
            break;
        }
    }

    if (!header_ok) {
        SDL_Log("scene_load: missing or bad racr_scene header in %s", path);
        fclose(f);
        return false;
    }

    scene_clear(scene);
    light_setup_reset_points(&scene->lights);

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        if (strncmp(line, "camera ", 7) == 0) {
            float px, py, pz, yaw, pitch, fov, asp, nearp, farp;
            if (sscanf(line + 7, "%f %f %f %f %f %f %f %f %f", &px, &py, &pz,
                        &yaw, &pitch, &fov, &asp, &nearp, &farp) == 9) {
                cam->position   = vec3_new(px, py, pz);
                cam->yaw        = yaw;
                cam->pitch      = pitch;
                cam->fov        = fov;
                cam->aspect     = asp;
                cam->near_plane = nearp;
                cam->far_plane  = farp;
            }
            continue;
        }

        if (strncmp(line, "render ", 7) == 0) {
            float aniso;
            if (sscanf(line + 7, "%f", &aniso) == 1) {
                if (aniso < 1.0f) aniso = 1.0f;
                if (aniso > scene->render_settings.anisotropy_max)
                    aniso = scene->render_settings.anisotropy_max;
                scene->render_settings.anisotropy_target = aniso;
            }
            continue;
        }

        if (strncmp(line, "directional_light ", 18) == 0) {
            float dx, dy, dz, r, g, b, intensity;
            if (sscanf(line + 18, "%f %f %f %f %f %f %f",
                       &dx, &dy, &dz, &r, &g, &b, &intensity) == 7) {
                scene->lights.directional.direction = vec3_normalize(vec3_new(dx, dy, dz));
                scene->lights.directional.color = vec3_new(r, g, b);
                scene->lights.directional.intensity = intensity;
            }
            continue;
        }

        if (strncmp(line, "point_light ", 12) == 0) {
            PointLight pl;
            int n = sscanf(line + 12, "%f %f %f %f %f %f %f %f %f %f %f",
                       &pl.position.x, &pl.position.y, &pl.position.z,
                       &pl.direction.x, &pl.direction.y, &pl.direction.z,
                       &pl.color.x, &pl.color.y, &pl.color.z,
                       &pl.intensity, &pl.range);
            if (n == 11) {
                (void)light_setup_add_point(&scene->lights, pl);
            } else {
                n = sscanf(line + 12, "%f %f %f %f %f %f %f %f",
                           &pl.position.x, &pl.position.y, &pl.position.z,
                           &pl.color.x, &pl.color.y, &pl.color.z,
                           &pl.intensity, &pl.range);
            }
            if (n == 8) {
                pl.direction = vec3_new(0.0f, -1.0f, 0.0f);
                (void)light_setup_add_point(&scene->lights, pl);
            }
            continue;
        }

        if (strncmp(line, "object ", 7) == 0) {
            char opath[SCENE_MESH_PATH_MAX];
            Transform tr = {
                .position = vec3_new(0.0f, 0.0f, 0.0f),
                .rotation = vec3_new(0.0f, 0.0f, 0.0f),
                .scale = vec3_new(1.0f, 1.0f, 1.0f)
            };
            float old_ry = 0.0f;
            float old_sc = 1.0f;
            int n = sscanf(line + 7, "%511s %f %f %f %f %f %f %f %f %f", opath,
                           &tr.position.x, &tr.position.y, &tr.position.z,
                           &tr.rotation.x, &tr.rotation.y, &tr.rotation.z,
                           &tr.scale.x, &tr.scale.y, &tr.scale.z);
            if (n != 10) {
                n = sscanf(line + 7, "%511s %f %f %f %f %f", opath,
                           &tr.position.x, &tr.position.y, &tr.position.z,
                           &old_ry, &old_sc);
            }
            if (n == 6) {
                tr.rotation = vec3_new(0.0f, old_ry, 0.0f);
                tr.scale = vec3_new(old_sc, old_sc, old_sc);
            } else if (n != 10) {
                SDL_Log("scene_load: bad object line, skipping");
                continue;
            }

            Vertex   *verts   = NULL;
            uint32_t *indices = NULL;
            ObjGroup *groups  = NULL;
            int nv = 0, ni = 0, ng = 0;
            if (!obj_load_grouped(opath, &verts, &nv, &indices, &ni, &groups, &ng)) {
                SDL_Log("scene_load: skip missing mesh %s", opath);
                continue;
            }
            Mesh m = mesh_create(verts, nv, indices, ni);
            scene_add(scene, m, opath, tr, verts, nv, indices, ni, groups, ng);
            free(verts);
            free(indices);
            free(groups);
            current_obj = &scene->objects[scene->count - 1];
            continue;
        }

        if (strncmp(line, "submaterial ", 12) == 0 && current_obj) {
            char name[OBJ_GROUP_NAME_MAX];
            Material mat = material_default();
            float r, g, b, a, metallic, roughness;
            char normal_path[256] = {0};
            int n = sscanf(line + 12, "%63s %f %f %f %f %f %f %255s",
                           name, &r, &g, &b, &a, &metallic, &roughness, normal_path);
            if (n >= 4) {
                mat.base_color.x = r;
                mat.base_color.y = g;
                mat.base_color.z = b;
                if (n >= 5) mat.base_color.w = a;
                if (n >= 6) mat.metallic = metallic;
                if (n >= 7) mat.roughness = roughness;
                if (n >= 8 && strcmp(normal_path, "-") != 0) {
                    strncpy(mat.normal_map_path, normal_path, sizeof(mat.normal_map_path) - 1);
                    mat.normal_map_path[sizeof(mat.normal_map_path) - 1] = '\0';
                }
                (void)scene_set_submesh_material(current_obj, name, mat);
            }
            continue;
        }

        if (strncmp(line, "physics ", 8) == 0 && current_obj) {
            float mass = current_obj->physics.mass;
            float restitution = current_obj->physics.restitution;
            float friction = current_obj->physics.friction;
            int enabled_int = current_obj->physics.enabled ? 1 : 0;
            int n = sscanf(line + 8, "%f %f %f %d", &mass, &restitution, &friction, &enabled_int);
            if (n >= 1) current_obj->physics.mass = (mass > 0.01f) ? mass : 0.01f;
            if (n >= 2) current_obj->physics.restitution = restitution;
            if (n >= 3) current_obj->physics.friction = friction;
            if (n >= 4) current_obj->physics.enabled = (enabled_int != 0);
            current_obj->physics.restitution = fmaxf(0.0f, fminf(1.0f, current_obj->physics.restitution));
            current_obj->physics.friction = fmaxf(0.0f, current_obj->physics.friction);
            continue;
        }
    }

    fclose(f);
    SDL_Log("Scene loaded: %s (%d objects)", path, scene->count);
    return true;
}
