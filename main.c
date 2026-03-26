#include "engine/camera.h"
#include "engine/debug_draw.h"
#include "engine/gl_funcs.h"
#include "engine/mesh.h"
#include "engine/nk_impl.h"
#include "engine/obj_loader.h"
#include "engine/pick.h"
#include "engine/scene.h"
#include "engine/scene_io.h"
#include "engine/shader.h"
#include "engine/texture.h"

#include "nuklear.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define OBJ_PATH        "assets/model.obj"
#define SCENE_SAVE_PATH "scenes/default.scene"

/* Radians per pixel at sensitivity 1.0 */
#define MOUSE_SENS 0.0025f
#define PAN_SENS 0.01f
#define PICK_SUBMESH_STRIDE 1024u
#define CLICK_DRAG_THRESHOLD 3.0f

typedef struct {
    bool open;
    int obj_idx;
    int sub_idx;
    float x;
    float y;
} ContextMenuState;

static void build_sorted_submesh_indices(const SceneObject *obj, int *indices) {
    for (int i = 0; i < obj->sub_count; i++) indices[i] = i;
    for (int i = 1; i < obj->sub_count; i++) {
        int key = indices[i];
        int j = i - 1;
        while (j >= 0) {
            int cmp = strcasecmp(obj->submeshes[indices[j]].name,
                                 obj->submeshes[key].name);
            if (cmp < 0 || (cmp == 0 && indices[j] < key)) break;
            indices[j + 1] = indices[j];
            j--;
        }
        indices[j + 1] = key;
    }
}

static void upload_lighting_uniforms(GLuint shader, const LightSetup *lights) {
    shader_set_vec3(shader, "u_dir_light_dir", lights->directional.direction);
    shader_set_vec3(shader, "u_dir_light_color", lights->directional.color);
    shader_set_float(shader, "u_dir_light_intensity", lights->directional.intensity);

    int point_count = lights->point_count;
    if (point_count > LIGHT_SHADER_POINT_MAX) point_count = LIGHT_SHADER_POINT_MAX;
    shader_set_int(shader, "u_point_count", point_count);

    for (int i = 0; i < point_count; i++) {
        char name[64];
        const PointLight *pl = &lights->points[i];
        snprintf(name, sizeof(name), "u_point_pos[%d]", i);
        shader_set_vec3(shader, name, pl->position);
        snprintf(name, sizeof(name), "u_point_color[%d]", i);
        shader_set_vec3(shader, name, pl->color);
        snprintf(name, sizeof(name), "u_point_intensity[%d]", i);
        shader_set_float(shader, name, pl->intensity);
        snprintf(name, sizeof(name), "u_point_range[%d]", i);
        shader_set_float(shader, name, pl->range);
    }
}

static int choose_lod_index(const SceneObject *obj, vec3 cam_pos) {
    if (!obj || obj->lod_count <= 1) return 0;
    vec3 delta = vec3_sub(obj->transform.position, cam_pos);
    float dist = vec3_length(delta);
    for (int i = 0; i < obj->lod_count; i++) {
        if (dist <= obj->lods[i].max_distance) return i;
    }
    return obj->lod_count - 1;
}

static void load_obj_into_scene(Scene *scene, const char *path) {
    Vertex   *verts   = NULL;
    uint32_t *indices = NULL;
    ObjGroup *groups = NULL;
    int nv = 0, ni = 0, ng = 0;

    if (!obj_load_grouped(path, &verts, &nv, &indices, &ni, &groups, &ng))
        return;

    scene_clear(scene);
    Mesh m = mesh_create(verts, nv, indices, ni);
    Transform t = {
        .position = vec3_new(0.0f, 0.0f, 0.0f),
        .rotation = vec3_new(0.0f, 0.0f, 0.0f),
        .scale = vec3_new(1.0f, 1.0f, 1.0f),
    };
    scene_add(scene, m, path, t, verts, nv, indices, ni, groups, ng);
    free(verts);
    free(indices);
    free(groups);
}

static bool decode_pick_id(uint32_t id, int *out_obj_idx, int *out_sub_idx) {
    if (id == 0) return false;
    id -= 1;
    *out_obj_idx = (int)(id / PICK_SUBMESH_STRIDE);
    *out_sub_idx = (int)(id % PICK_SUBMESH_STRIDE);
    return true;
}

static bool pick_scene_at(const Scene *scene, const Camera *cam, GLuint pick_shader,
                          PickContext *pick_ctx, int mouse_x, int mouse_y,
                          int window_w, int window_h,
                          int *out_obj_idx, int *out_sub_idx) {
    pick_bind(pick_ctx);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_use(pick_shader);
    mat4 view = camera_view(cam);
    mat4 proj = camera_projection(cam);
    shader_set_mat4(pick_shader, "u_view", &view);
    shader_set_mat4(pick_shader, "u_proj", &proj);

    for (int i = 0; i < scene->count; i++) {
        const SceneObject *obj = &scene->objects[i];
        mat4 model = scene_object_model(obj);
        shader_set_mat4(pick_shader, "u_model", &model);
        for (int s = 0; s < obj->sub_count; s++) {
            uint32_t pick_id = 1u + (uint32_t)i * PICK_SUBMESH_STRIDE + (uint32_t)s;
            float rgb[3];
            pick_id_to_rgb(pick_id, rgb);
            shader_set_vec3(pick_shader, "u_pick_color", vec3_new(rgb[0], rgb[1], rgb[2]));
            mesh_draw_range(&obj->mesh, obj->submeshes[s].first_index,
                            obj->submeshes[s].index_count);
        }
    }

    uint32_t read_id = pick_read_id_at(pick_ctx, mouse_x, mouse_y);
    pick_unbind();
    glViewport(0, 0, window_w, window_h);
    if (!decode_pick_id(read_id, out_obj_idx, out_sub_idx)) return false;
    if (*out_obj_idx < 0 || *out_obj_idx >= scene->count) return false;
    if (*out_sub_idx < 0 || *out_sub_idx >= scene->objects[*out_obj_idx].sub_count) return false;
    return true;
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

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to load OpenGL via glad");
        SDL_GL_DestroyContext(gl_ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    int window_w = 800;
    int window_h = 600;

    GLuint shader_main = shader_create("shaders/basic.vert", "shaders/basic.frag");
    GLuint shader_pick = shader_create("shaders/pick.vert", "shaders/pick.frag");
    if (!shader_main || !shader_pick) {
        SDL_Log("Shader creation failed");
        shader_destroy(shader_main);
        shader_destroy(shader_pick);
        SDL_GL_DestroyContext(gl_ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    PickContext pick_ctx;
    if (!pick_init(&pick_ctx, window_w, window_h)) {
        SDL_Log("Failed to initialize picking framebuffer");
        shader_destroy(shader_main);
        shader_destroy(shader_pick);
        SDL_GL_DestroyContext(gl_ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    struct nk_context *ui = nk_impl_init(window);
    if (!ui) {
        SDL_Log("Failed to initialize nuklear");
        pick_destroy(&pick_ctx);
        shader_destroy(shader_main);
        shader_destroy(shader_pick);
        SDL_GL_DestroyContext(gl_ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Scene scene;
    scene_init(&scene);
    render_settings_detect_gl_caps(&scene.render_settings);
    GLuint flat_normal_tex = texture_create_flat_normal(&scene.render_settings);
    DebugLines point_light_gizmo;
    (void)debug_lines_init_point_light(&point_light_gizmo);

    Camera cam = camera_default(800.0f / 600.0f);
    if (!scene_load(&scene, &cam, SCENE_SAVE_PATH))
        load_obj_into_scene(&scene, OBJ_PATH);
    camera_clamp_angles(&cam);
    float move_speed = 0.15f;

    bool running = true;
    bool lmb_held = false;
    bool rmb_held = false;
    bool rmb_dragging = false;
    float rmb_down_x = 0.0f;
    float rmb_down_y = 0.0f;
    bool ui_capture_mouse = false;
    int selected_point_light = -1;
    ContextMenuState menu = {0};
    Uint64 perf_freq = SDL_GetPerformanceFrequency();
    Uint64 perf_prev = SDL_GetPerformanceCounter();
    float fps_ema = 0.0f;

    SDL_Event ev;
    while (running) {
        int frame_vertices = 0;
        int frame_draw_calls = 0;
        nk_impl_input_begin();
        while (SDL_PollEvent(&ev)) {
            nk_impl_handle_event(&ev);

            switch (ev.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                switch (ev.key.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    if (menu.open) menu.open = false;
                    else running = false;
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
                    camera_clamp_angles(&cam);
                    break;
                case SDL_SCANCODE_RIGHT:
                    cam.yaw += 0.05f;
                    camera_clamp_angles(&cam);
                    break;
                case SDL_SCANCODE_UP:
                    cam.pitch += 0.05f;
                    camera_clamp_angles(&cam);
                    break;
                case SDL_SCANCODE_DOWN:
                    cam.pitch -= 0.05f;
                    camera_clamp_angles(&cam);
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
                if (ev.button.button == SDL_BUTTON_LEFT && menu.open && !ui_capture_mouse) {
                    menu.open = false;
                } else if (!ui_capture_mouse && ev.button.button == SDL_BUTTON_LEFT && !lmb_held) {
                    lmb_held = true;
                    SDL_SetWindowRelativeMouseMode(window, true);
                } else if (!ui_capture_mouse && ev.button.button == SDL_BUTTON_RIGHT && !rmb_held) {
                    rmb_held = true;
                    rmb_dragging = false;
                    rmb_down_x = ev.button.x;
                    rmb_down_y = ev.button.y;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    lmb_held = false;
                    if (!rmb_held || !rmb_dragging)
                        SDL_SetWindowRelativeMouseMode(window, false);
                } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                    bool was_dragging = rmb_dragging;
                    rmb_held = false;
                    rmb_dragging = false;
                    if (was_dragging) {
                        if (!lmb_held)
                            SDL_SetWindowRelativeMouseMode(window, false);
                    } else if (!ui_capture_mouse) {
                        int hit_obj = -1, hit_sub = -1;
                        if (pick_scene_at(&scene, &cam, shader_pick, &pick_ctx,
                                          (int)ev.button.x, (int)ev.button.y,
                                          window_w, window_h, &hit_obj, &hit_sub)) {
                            menu.open = true;
                            menu.obj_idx = hit_obj;
                            menu.sub_idx = hit_sub;
                            menu.x = ev.button.x;
                            menu.y = ev.button.y;
                        } else {
                            menu.open = false;
                        }
                    }
                    if (!lmb_held && !rmb_held)
                        SDL_SetWindowRelativeMouseMode(window, false);
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (lmb_held && !ui_capture_mouse) {
                    camera_apply_mouse_look(&cam, ev.motion.xrel,
                                            ev.motion.yrel, MOUSE_SENS);
                } else if (rmb_held && !ui_capture_mouse) {
                    if (!rmb_dragging) {
                        float dx = ev.motion.x - rmb_down_x;
                        float dy = ev.motion.y - rmb_down_y;
                        if (fabsf(dx) > CLICK_DRAG_THRESHOLD || fabsf(dy) > CLICK_DRAG_THRESHOLD) {
                            rmb_dragging = true;
                            if (!lmb_held) SDL_SetWindowRelativeMouseMode(window, true);
                        }
                    }
                    if (rmb_dragging) {
                        camera_pan(&cam, -ev.motion.xrel * PAN_SENS,
                                   ev.motion.yrel * PAN_SENS);
                    }
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if (!ui_capture_mouse) {
                    move_speed += ev.wheel.y * 0.04f;
                    if (move_speed < 0.03f) move_speed = 0.03f;
                    if (move_speed > 0.6f) move_speed = 0.6f;
                }
                break;
            case SDL_EVENT_WINDOW_RESIZED: {
                int w = ev.window.data1, h = ev.window.data2;
                if (h > 0) {
                    window_w = w;
                    window_h = h;
                    glViewport(0, 0, w, h);
                    cam.aspect = (float)w / (float)h;
                    (void)pick_resize(&pick_ctx, w, h);
                }
                break;
            }
            default:
                break;
            }
        }
        nk_impl_input_end();

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
        if (!ui_capture_mouse && (mf != 0.0f || mr != 0.0f || mu != 0.0f))
            camera_move_fps(&cam, mf, mr, mu, move_speed);

        Uint64 perf_now = SDL_GetPerformanceCounter();
        float delta_s = (float)(perf_now - perf_prev) / (float)perf_freq;
        perf_prev = perf_now;
        float fps_now = (delta_s > 1e-6f) ? (1.0f / delta_s) : 0.0f;
        if (fps_ema <= 0.0f) fps_ema = fps_now;
        fps_ema = fps_ema * 0.95f + fps_now * 0.05f;

        glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /* nk_sdl_render disables depth testing; restore for 3D scene pass. */
        glEnable(GL_DEPTH_TEST);

        if (scene.count > 0) {
            shader_use(shader_main);

            mat4 view = camera_view(&cam);
            mat4 proj = camera_projection(&cam);
            vec3 cam_pos = camera_position(&cam);

            shader_set_mat4(shader_main, "u_view", &view);
            shader_set_mat4(shader_main, "u_proj", &proj);
            shader_set_vec3(shader_main, "u_cam_pos", cam_pos);
            shader_set_int(shader_main, "u_normal_map", 0);
            upload_lighting_uniforms(shader_main, &scene.lights);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            for (int i = 0; i < scene.count; i++) {
                SceneObject *obj = &scene.objects[i];
                mat4 model = scene_object_model(obj);
                shader_set_mat4(shader_main, "u_model", &model);

                int lod_idx = choose_lod_index(obj, cam_pos);
                if (lod_idx <= 0) {
                    for (int s = 0; s < obj->sub_count; s++) {
                        SubMesh *sm = &obj->submeshes[s];
                        shader_set_vec4(shader_main, "u_base_color", sm->material.base_color);
                        shader_set_float(shader_main, "u_metallic", sm->material.metallic);
                        shader_set_float(shader_main, "u_roughness", sm->material.roughness);
                        if (sm->material.normal_map_tex == 0 && sm->material.normal_map_path[0]) {
                            sm->material.normal_map_tex =
                                texture_load_2d(sm->material.normal_map_path, &scene.render_settings);
                        }
                        GLuint ntex = sm->material.normal_map_tex ? sm->material.normal_map_tex : flat_normal_tex;
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, ntex);
                        shader_set_int(shader_main, "u_has_normal_map", sm->material.normal_map_tex ? 1 : 0);
                        mesh_draw_range(&obj->mesh, sm->first_index, sm->index_count);
                        frame_vertices += sm->index_count;
                        frame_draw_calls++;
                    }
                } else {
                    const LODLevel *lod = &obj->lods[lod_idx];
                    shader_set_vec4(shader_main, "u_base_color", vec4_new(0.75f, 0.75f, 0.75f, 1.0f));
                    shader_set_float(shader_main, "u_metallic", 0.0f);
                    shader_set_float(shader_main, "u_roughness", 0.9f);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, flat_normal_tex);
                    shader_set_int(shader_main, "u_has_normal_map", 0);
                    mesh_draw(&lod->mesh);
                    frame_vertices += lod->index_count;
                    frame_draw_calls++;
                }
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_BLEND);

            for (int i = 0; i < scene.lights.point_count; i++) {
                const PointLight *pl = &scene.lights.points[i];
                float gizmo_scale = fminf(fmaxf(pl->range * 0.05f, 0.2f), 2.5f);
                debug_lines_draw(&point_light_gizmo, shader_pick, &view, &proj,
                                 pl->position, pl->direction, gizmo_scale, pl->color);
                frame_draw_calls++;
            }
        }

        if (menu.open && menu.obj_idx >= 0 && menu.obj_idx < scene.count) {
            SceneObject *obj = &scene.objects[menu.obj_idx];
            if (menu.sub_idx < 0 || menu.sub_idx >= obj->sub_count) menu.sub_idx = 0;

            int flags = NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE;
            if (nk_begin(ui, "Object Context", nk_rect(menu.x, menu.y, 360, 420), flags)) {
                nk_layout_row_dynamic(ui, 20, 1);
                nk_label(ui, obj->mesh_path, NK_TEXT_LEFT);
                nk_layout_row_dynamic(ui, 20, 1);
                nk_label(ui, "Transform", NK_TEXT_LEFT);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.position.x = nk_slide_float(ui, -100.0f, obj->transform.position.x, 100.0f, 0.05f);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.position.y = nk_slide_float(ui, -100.0f, obj->transform.position.y, 100.0f, 0.05f);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.position.z = nk_slide_float(ui, -100.0f, obj->transform.position.z, 100.0f, 0.05f);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.rotation.x = nk_slide_float(ui, -3.14159f, obj->transform.rotation.x, 3.14159f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.rotation.y = nk_slide_float(ui, -3.14159f, obj->transform.rotation.y, 3.14159f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.rotation.z = nk_slide_float(ui, -3.14159f, obj->transform.rotation.z, 3.14159f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.scale.x = nk_slide_float(ui, 0.01f, obj->transform.scale.x, 20.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.scale.y = nk_slide_float(ui, 0.01f, obj->transform.scale.y, 20.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                obj->transform.scale.z = nk_slide_float(ui, 0.01f, obj->transform.scale.z, 20.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                nk_label(ui, "Components", NK_TEXT_LEFT);

                int *sorted_indices = malloc((size_t)obj->sub_count * sizeof(int));
                if (sorted_indices) {
                    build_sorted_submesh_indices(obj, sorted_indices);
                }
                for (int oi = 0; oi < obj->sub_count; oi++) {
                    int s = sorted_indices ? sorted_indices[oi] : oi;
                    SubMesh *sm = &obj->submeshes[s];
                    if (nk_tree_push_id(ui, NK_TREE_NODE, sm->name, NK_MINIMIZED, s)) {
                        Material edited = sm->material;
                        char color_label[128];

                        nk_layout_row_dynamic(ui, 18, 1);
                        snprintf(color_label, sizeof(color_label), "R: %.3f", edited.base_color.x);
                        nk_label(ui, color_label, NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ui, 20, 1);
                        edited.base_color.x = nk_slide_float(ui, 0.0f, edited.base_color.x, 1.0f, 0.002f);

                        nk_layout_row_dynamic(ui, 18, 1);
                        snprintf(color_label, sizeof(color_label), "G: %.3f", edited.base_color.y);
                        nk_label(ui, color_label, NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ui, 20, 1);
                        edited.base_color.y = nk_slide_float(ui, 0.0f, edited.base_color.y, 1.0f, 0.002f);

                        nk_layout_row_dynamic(ui, 18, 1);
                        snprintf(color_label, sizeof(color_label), "B: %.3f", edited.base_color.z);
                        nk_label(ui, color_label, NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ui, 20, 1);
                        edited.base_color.z = nk_slide_float(ui, 0.0f, edited.base_color.z, 1.0f, 0.002f);

                        nk_layout_row_dynamic(ui, 18, 1);
                        snprintf(color_label, sizeof(color_label), "A: %.3f", edited.base_color.w);
                        nk_label(ui, color_label, NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ui, 20, 1);
                        edited.base_color.w = nk_slide_float(ui, 0.0f, edited.base_color.w, 1.0f, 0.002f);

                        nk_layout_row_dynamic(ui, 18, 1);
                        snprintf(color_label, sizeof(color_label), "Metallic: %.3f", edited.metallic);
                        nk_label(ui, color_label, NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ui, 20, 1);
                        edited.metallic = nk_slide_float(ui, 0.0f, edited.metallic, 1.0f, 0.002f);

                        nk_layout_row_dynamic(ui, 18, 1);
                        snprintf(color_label, sizeof(color_label), "Roughness: %.3f", edited.roughness);
                        nk_label(ui, color_label, NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ui, 20, 1);
                        edited.roughness = nk_slide_float(ui, 0.04f, edited.roughness, 1.0f, 0.002f);

                        nk_layout_row_dynamic(ui, 18, 1);
                        snprintf(color_label, sizeof(color_label), "Normal map: %s",
                                 edited.normal_map_path[0] ? edited.normal_map_path : "(none)");
                        nk_label(ui, color_label, NK_TEXT_LEFT);
                        nk_layout_row_dynamic(ui, 28, 2);
                        if (nk_button_label(ui, "Use default")) {
                            edited.normal_map_path[0] = '\0';
                            edited.normal_map_tex = 0;
                        }
                        if (nk_button_label(ui, "Use assets/normal.bmp")) {
                            snprintf(edited.normal_map_path, sizeof(edited.normal_map_path),
                                     "assets/normal.bmp");
                            edited.normal_map_tex = 0;
                        }

                        scene_set_submesh_material_index(obj, s, edited);
                        nk_tree_pop(ui);
                    }
                }
                free(sorted_indices);
            }
            if (nk_window_is_closed(ui, "Object Context")) menu.open = false;
            nk_end(ui);
        }

        if (nk_begin(ui, "Lighting and Render", nk_rect(20, 20, 380, 520),
                     NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE)) {
            nk_layout_row_dynamic(ui, 20, 1);
            nk_label(ui, "Directional Light", NK_TEXT_LEFT);

            nk_layout_row_dynamic(ui, 20, 1);
            scene.lights.directional.direction.x =
                nk_slide_float(ui, -1.0f, scene.lights.directional.direction.x, 1.0f, 0.01f);
            nk_layout_row_dynamic(ui, 20, 1);
            scene.lights.directional.direction.y =
                nk_slide_float(ui, -1.0f, scene.lights.directional.direction.y, 1.0f, 0.01f);
            nk_layout_row_dynamic(ui, 20, 1);
            scene.lights.directional.direction.z =
                nk_slide_float(ui, -1.0f, scene.lights.directional.direction.z, 1.0f, 0.01f);
            scene.lights.directional.direction = vec3_normalize(scene.lights.directional.direction);

            nk_layout_row_dynamic(ui, 20, 1);
            scene.lights.directional.color.x =
                nk_slide_float(ui, 0.0f, scene.lights.directional.color.x, 1.0f, 0.01f);
            nk_layout_row_dynamic(ui, 20, 1);
            scene.lights.directional.color.y =
                nk_slide_float(ui, 0.0f, scene.lights.directional.color.y, 1.0f, 0.01f);
            nk_layout_row_dynamic(ui, 20, 1);
            scene.lights.directional.color.z =
                nk_slide_float(ui, 0.0f, scene.lights.directional.color.z, 1.0f, 0.01f);
            nk_layout_row_dynamic(ui, 20, 1);
            scene.lights.directional.intensity =
                nk_slide_float(ui, 0.0f, scene.lights.directional.intensity, 8.0f, 0.01f);

            nk_layout_row_dynamic(ui, 20, 1);
            nk_label(ui, "Point Lights", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ui, 28, 2);
            if (nk_button_label(ui, "Add")) {
                PointLight p = {
                    .position = vec3_new(2.0f, 2.0f, 2.0f),
                    .direction = vec3_new(0.0f, -1.0f, 0.0f),
                    .color = vec3_new(1.0f, 1.0f, 1.0f),
                    .intensity = 2.0f,
                    .range = 10.0f,
                };
                int idx = light_setup_add_point(&scene.lights, p);
                if (idx >= 0) selected_point_light = idx;
            }
            if (nk_button_label(ui, "Remove")) {
                light_setup_remove_point(&scene.lights, selected_point_light);
                if (selected_point_light >= scene.lights.point_count)
                    selected_point_light = scene.lights.point_count - 1;
            }

            for (int i = 0; i < scene.lights.point_count; i++) {
                char pl_name[32];
                nk_bool selected = (i == selected_point_light) ? 1 : 0;
                snprintf(pl_name, sizeof(pl_name), "Point %d", i + 1);
                nk_layout_row_dynamic(ui, 20, 1);
                if (nk_selectable_label(ui, pl_name, NK_TEXT_LEFT, &selected) && selected) {
                    selected_point_light = i;
                }
            }

            if (selected_point_light >= 0 && selected_point_light < scene.lights.point_count) {
                PointLight *pl = &scene.lights.points[selected_point_light];
                nk_layout_row_dynamic(ui, 20, 1);
                nk_label(ui, "Selected Point", NK_TEXT_LEFT);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->position.x = nk_slide_float(ui, -30.0f, pl->position.x, 30.0f, 0.05f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->position.y = nk_slide_float(ui, -30.0f, pl->position.y, 30.0f, 0.05f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->position.z = nk_slide_float(ui, -30.0f, pl->position.z, 30.0f, 0.05f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->direction.x = nk_slide_float(ui, -1.0f, pl->direction.x, 1.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->direction.y = nk_slide_float(ui, -1.0f, pl->direction.y, 1.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->direction.z = nk_slide_float(ui, -1.0f, pl->direction.z, 1.0f, 0.01f);
                pl->direction = vec3_normalize(pl->direction);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->color.x = nk_slide_float(ui, 0.0f, pl->color.x, 1.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->color.y = nk_slide_float(ui, 0.0f, pl->color.y, 1.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->color.z = nk_slide_float(ui, 0.0f, pl->color.z, 1.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->intensity = nk_slide_float(ui, 0.0f, pl->intensity, 8.0f, 0.01f);
                nk_layout_row_dynamic(ui, 20, 1);
                pl->range = nk_slide_float(ui, 0.1f, pl->range, 100.0f, 0.05f);
            }

            nk_layout_row_dynamic(ui, 20, 1);
            nk_label(ui, "Anisotropic Filtering", NK_TEXT_LEFT);
            if (scene.render_settings.anisotropy_supported) {
                nk_layout_row_dynamic(ui, 20, 1);
                scene.render_settings.anisotropy_target =
                    nk_slide_float(ui, 1.0f, scene.render_settings.anisotropy_target,
                                   scene.render_settings.anisotropy_max, 0.1f);
            } else {
                nk_layout_row_dynamic(ui, 20, 1);
                nk_label(ui, "Not supported by this OpenGL driver", NK_TEXT_LEFT);
            }
        }
        nk_end(ui);

        if (nk_begin(ui, "##perf_overlay",
                     nk_rect((float)window_w - 210.0f, 10.0f, 200.0f, 90.0f),
                     NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
            char buf[128];
            nk_layout_row_dynamic(ui, 18, 1);
            snprintf(buf, sizeof(buf), "FPS: %.1f", fps_ema);
            nk_label(ui, buf, NK_TEXT_LEFT);
            nk_layout_row_dynamic(ui, 18, 1);
            snprintf(buf, sizeof(buf), "Vertices: %d", frame_vertices);
            nk_label(ui, buf, NK_TEXT_LEFT);
            nk_layout_row_dynamic(ui, 18, 1);
            snprintf(buf, sizeof(buf), "Draw calls: %d", frame_draw_calls);
            nk_label(ui, buf, NK_TEXT_LEFT);
        }
        nk_end(ui);

        ui_capture_mouse = nk_window_is_any_hovered(ui) || nk_item_is_any_active(ui);
        nk_impl_render();

        SDL_GL_SwapWindow(window);
    }

    scene_destroy(&scene);
    debug_lines_destroy(&point_light_gizmo);
    texture_destroy(flat_normal_tex);
    nk_impl_shutdown();
    pick_destroy(&pick_ctx);
    shader_destroy(shader_main);
    shader_destroy(shader_pick);
    SDL_SetWindowRelativeMouseMode(window, false);
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
