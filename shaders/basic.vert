#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec4 a_tangent;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_world_pos;
out vec3 v_normal;
out vec2 v_uv;
out mat3 v_tbn;

void main() {
    vec4 world = u_model * vec4(a_position, 1.0);
    v_world_pos = world.xyz;
    mat3 normal_mat = mat3(transpose(inverse(u_model)));
    v_normal = normalize(normal_mat * a_normal);
    v_uv = a_uv;
    vec3 tangent_world = normalize(normal_mat * a_tangent.xyz);
    vec3 bitangent_world = normalize(cross(v_normal, tangent_world) * a_tangent.w);
    v_tbn = mat3(tangent_world, bitangent_world, v_normal);
    gl_Position = u_proj * u_view * world;
}
