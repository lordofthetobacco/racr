#version 330 core

in vec3 v_world_pos;
in vec3 v_normal;
in vec2 v_uv;

uniform vec3 u_cam_pos;

out vec4 frag_color;

void main() {
    vec3 light_dir = normalize(vec3(0.5, -1.0, -0.3));
    vec3 light_col = vec3(1.0, 0.98, 0.92);

    vec3 base_col = vec3(0.75, 0.75, 0.75);

    vec3 n = normalize(v_normal);

    /* ambient */
    vec3 ambient = 0.15 * base_col;

    /* diffuse */
    float diff = max(dot(n, -light_dir), 0.0);
    vec3 diffuse = diff * light_col * base_col;

    /* specular (Blinn-Phong) */
    vec3 view_dir = normalize(u_cam_pos - v_world_pos);
    vec3 half_dir = normalize(view_dir - light_dir);
    float spec = pow(max(dot(n, half_dir), 0.0), 64.0);
    vec3 specular = spec * light_col * 0.5;

    frag_color = vec4(ambient + diffuse + specular, 1.0);
}
