#include "engine/shader.h"
#include "engine/mesh.h"
#include "engine/obj_loader.h"
#include "engine/scene.h"
#include "engine/scene_io.h"
#include "engine/camera.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stdbool.h>
#include <stdlib.h>

#define OBJ_PATH        "assets/model.obj"
#define SCENE_SAVE_PATH "scenes/default.scene"

/* Radians per pixel at sensitivity 1.0 */
#define MOUSE_SENS 0.0025f

static void load_obj_into_scene(Scene *scene, const char *path) {
    Vertex   *verts   = NULL;
    uint32_t *indices = NULL;
    int nv = 0, ni = 0;

    if (!obj_load(path, &verts, &nv, &indices, &ni))
        return;

    scene_clear(scene);
    Mesh m = mesh_create(verts, nv, indices, ni);
    scene_add(scene, m, path, vec3_new(0, 0, 0), 0.0f, 1.0f);
    free(verts);
    free(indices);
}

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window *window = SDL_CreateWindow(
        "racr", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        SDL_Log("GL context failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, gl_ctx);
    SDL_GL_SetSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);

    GLuint shader = shader_create("shaders/basic.vert", "shaders/basic.frag");
    if (!shader) {
        SDL_Log("Shader creation failed");
        SDL_GL_DestroyContext(gl_ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Scene scene;
    scene_init(&scene);

    Camera cam = camera_default(800.0f / 600.0f);
    if (!scene_load(&scene, &cam, SCENE_SAVE_PATH))
        load_obj_into_scene(&scene, OBJ_PATH);
    float move_speed = 0.15f;

    #define PAN_SENS 0.01f

    bool running = true;
    bool lmb_held = false;
    bool rmb_held = false;

    SDL_Event ev;
    while (running) {
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                switch (ev.key.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    running = false;
                    break;
                case SDL_SCANCODE_L:
                    load_obj_into_scene(&scene, OBJ_PATH);
                    break;
                case SDL_SCANCODE_F5:
                    (void)scene_save(&scene, &cam, SCENE_SAVE_PATH);
                    break;
                case SDL_SCANCODE_F6:
                    (void)scene_load(&scene, &cam, SCENE_SAVE_PATH);
                    break;
                case SDL_SCANCODE_LEFT:
                    cam.yaw -= 0.05f;
                    break;
                case SDL_SCANCODE_RIGHT:
                    cam.yaw += 0.05f;
                    break;
                case SDL_SCANCODE_UP:
                    cam.pitch += 0.05f;
                    if (cam.pitch > 1.48f) cam.pitch = 1.48f;
                    break;
                case SDL_SCANCODE_DOWN:
                    cam.pitch -= 0.05f;
                    if (cam.pitch < -1.48f) cam.pitch = -1.48f;
                    break;
                case SDL_SCANCODE_EQUALS:
                    move_speed += 0.03f;
                    if (move_speed > 0.6f) move_speed = 0.6f;
                    break;
                case SDL_SCANCODE_MINUS:
                    move_speed -= 0.03f;
                    if (move_speed < 0.03f) move_speed = 0.03f;
                    break;
                default:
                    break;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (ev.button.button == SDL_BUTTON_LEFT && !lmb_held) {
                    lmb_held = true;
                    SDL_SetWindowRelativeMouseMode(window, true);
                } else if (ev.button.button == SDL_BUTTON_RIGHT && !rmb_held) {
                    rmb_held = true;
                    if (!lmb_held)
                        SDL_SetWindowRelativeMouseMode(window, true);
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    lmb_held = false;
                    if (!rmb_held)
                        SDL_SetWindowRelativeMouseMode(window, false);
                } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                    rmb_held = false;
                    if (!lmb_held)
                        SDL_SetWindowRelativeMouseMode(window, false);
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (lmb_held) {
                    camera_apply_mouse_look(&cam, ev.motion.xrel,
                                            ev.motion.yrel, MOUSE_SENS);
                } else if (rmb_held) {
                    camera_pan(&cam, -ev.motion.xrel * PAN_SENS,
                                      ev.motion.yrel * PAN_SENS);
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                move_speed += ev.wheel.y * 0.04f;
                if (move_speed < 0.03f) move_speed = 0.03f;
                if (move_speed > 0.6f) move_speed = 0.6f;
                break;
            case SDL_EVENT_WINDOW_RESIZED: {
                int w = ev.window.data1, h = ev.window.data2;
                if (h > 0) {
                    glViewport(0, 0, w, h);
                    cam.aspect = (float)w / (float)h;
                }
                break;
            }
            default:
                break;
            }
        }

        int nk = 0;
        const bool *keys = SDL_GetKeyboardState(&nk);
        float mf = 0.0f, mr = 0.0f, mu = 0.0f;
        if (keys[SDL_SCANCODE_W]) mf += 1.0f;
        if (keys[SDL_SCANCODE_S]) mf -= 1.0f;
        if (keys[SDL_SCANCODE_D]) mr += 1.0f;
        if (keys[SDL_SCANCODE_A]) mr -= 1.0f;
        if (keys[SDL_SCANCODE_SPACE]) mu += 1.0f;
        if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_C])
            mu -= 1.0f;
        if (mf != 0.0f || mr != 0.0f || mu != 0.0f)
            camera_move_fps(&cam, mf, mr, mu, move_speed);

        glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (scene.count > 0) {
            shader_use(shader);

            mat4 view = camera_view(&cam);
            mat4 proj = camera_projection(&cam);
            vec3 cam_pos = camera_position(&cam);

            shader_set_mat4(shader, "u_view", &view);
            shader_set_mat4(shader, "u_proj", &proj);
            shader_set_vec3(shader, "u_cam_pos", cam_pos);

            for (int i = 0; i < scene.count; i++) {
                mat4 model = scene_object_model(&scene.objects[i]);
                shader_set_mat4(shader, "u_model", &model);
                mesh_draw(&scene.objects[i].mesh);
            }
        }

        SDL_GL_SwapWindow(window);
    }

    scene_destroy(&scene);
    shader_destroy(shader);
    SDL_SetWindowRelativeMouseMode(window, false);
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
