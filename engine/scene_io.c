#include "scene_io.h"
#include "obj_loader.h"
#include "mesh.h"
#include <SDL3/SDL.h>
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

    for (int i = 0; i < scene->count; i++) {
        const SceneObject *o = &scene->objects[i];
        const char *mp = o->mesh_path[0] ? o->mesh_path : "assets/model.obj";
        fprintf(f, "object %s %g %g %g %g %g\n", mp, (double)o->position.x,
                (double)o->position.y, (double)o->position.z,
                (double)o->rotation_y, (double)o->scale);
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

        if (strncmp(line, "object ", 7) == 0) {
            char opath[SCENE_MESH_PATH_MAX];
            float px, py, pz, ry, sc;
            if (sscanf(line + 7, "%511s %f %f %f %f %f", opath, &px, &py, &pz,
                       &ry, &sc) != 6) {
                SDL_Log("scene_load: bad object line, skipping");
                continue;
            }

            Vertex   *verts   = NULL;
            uint32_t *indices = NULL;
            int nv = 0, ni = 0;
            if (!obj_load(opath, &verts, &nv, &indices, &ni)) {
                SDL_Log("scene_load: skip missing mesh %s", opath);
                continue;
            }
            Mesh m = mesh_create(verts, nv, indices, ni);
            free(verts);
            free(indices);
            scene_add(scene, m, opath, vec3_new(px, py, pz), ry, sc);
        }
    }

    fclose(f);
    SDL_Log("Scene loaded: %s (%d objects)", path, scene->count);
    return true;
}
