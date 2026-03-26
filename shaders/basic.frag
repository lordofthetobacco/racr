#version 330 core

in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_uv;
in mat3 v_tbn;

uniform vec3 u_cam_pos;
uniform vec4 u_base_color;
uniform float u_metallic;
uniform float u_roughness;
uniform sampler2D u_normal_map;
uniform int u_has_normal_map;

uniform vec3 u_dir_light_dir;
uniform vec3 u_dir_light_color;
uniform float u_dir_light_intensity;

uniform int u_point_count;
#define MAX_POINT_LIGHTS 8
uniform vec3 u_point_pos[MAX_POINT_LIGHTS];
uniform vec3 u_point_color[MAX_POINT_LIGHTS];
uniform float u_point_intensity[MAX_POINT_LIGHTS];
uniform float u_point_range[MAX_POINT_LIGHTS];

out vec4 frag_color;

void main() {
    vec3 base_col = u_base_color.rgb;
    float metallic = clamp(u_metallic, 0.0, 1.0);
    float roughness = clamp(u_roughness, 0.04, 1.0);
    vec3 n = normalize(v_normal);
    if (u_has_normal_map == 1) {
        vec3 normal_tex = texture(u_normal_map, v_uv).xyz * 2.0 - 1.0;
        n = normalize(v_tbn * normal_tex);
    }
    vec3 view_dir = normalize(u_cam_pos - v_world_pos);
    vec3 f0 = mix(vec3(0.04), base_col, metallic);
    float spec_power = mix(128.0, 8.0, roughness);
    vec3 lit = vec3(0.0);

    vec3 dir_l = normalize(-u_dir_light_dir);
    float dir_diff = max(dot(n, dir_l), 0.0);
    vec3 dir_h = normalize(view_dir + dir_l);
    float dir_spec = pow(max(dot(n, dir_h), 0.0), spec_power);
    vec3 dir_color = u_dir_light_color * u_dir_light_intensity;
    lit += (1.0 - metallic) * dir_diff * base_col * dir_color;
    lit += dir_spec * f0 * dir_color;

    for (int i = 0; i < u_point_count && i < MAX_POINT_LIGHTS; i++) {
        vec3 light_vec = u_point_pos[i] - v_world_pos;
        float dist = length(light_vec);
        vec3 point_l = dist > 1e-5 ? light_vec / dist : vec3(0.0, 1.0, 0.0);
        float p_diff = max(dot(n, point_l), 0.0);
        vec3 point_h = normalize(view_dir + point_l);
        float p_spec = pow(max(dot(n, point_h), 0.0), spec_power);

        float range = max(u_point_range[i], 0.001);
        float attenuation = 1.0 / (1.0 + (dist * dist) / (range * range));
        vec3 point_color = u_point_color[i] * u_point_intensity[i] * attenuation;

        lit += (1.0 - metallic) * p_diff * base_col * point_color;
        lit += p_spec * f0 * point_color;
    }

    vec3 ambient = 0.08 * base_col;
    vec3 color = ambient + lit;
    frag_color = vec4(color, u_base_color.a);
}
