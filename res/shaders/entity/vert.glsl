#version 410 core
#include "../common/lights.glsl"

// Per vertex data
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;

out VertexOut {
    vec3 ws_position;
    vec3 ws_normal;
    vec3 ws_view_dir;
    vec2 texture_coordinate;
} vertex_out;

// Per instance data
uniform float texture_scale;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;
uniform vec3 ws_view_position;
uniform mat4 projection_view_matrix;

void main() {
    vec3 ws_pos = (model_matrix * vec4(vertex_position, 1.0)).xyz;
    vec3 ws_nrm = normalize(normal_matrix * normal);
    vec3 ws_vdir = normalize(ws_view_position - ws_pos);

    vertex_out.ws_position = ws_pos;
    vertex_out.ws_normal = ws_nrm;
    vertex_out.ws_view_dir = ws_vdir;
    vertex_out.texture_coordinate = texture_coordinate * texture_scale;

    gl_Position = projection_view_matrix * vec4(ws_pos, 1.0);
}