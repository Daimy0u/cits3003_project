#version 410 core
#include "../common/lights.glsl"
#include "../common/maths.glsl"

// Per vertex data
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;
layout(location = 3) in vec4 bone_weights;
layout(location = 4) in uvec4 bone_indices;

out VertexOut {
    vec3 ws_position;
    vec3 ws_normal;
    vec3 ws_view_dir;
    vec2 texture_coordinate;
} vertex_out;
// Per instance data
uniform mat4 model_matrix;

// Animation Data
uniform mat4 bone_transforms[BONE_TRANSFORMS];

// Global data
uniform vec3 ws_view_position;
uniform mat4 projection_view_matrix;

//Task F
uniform float texture_scale;

void main() {
    // Transform vertices
    float sum = dot(bone_weights, vec4(1.0f));

    mat4 bone_transform =
        bone_weights[0] * bone_transforms[bone_indices[0]]
        + bone_weights[1] * bone_transforms[bone_indices[1]]
        + bone_weights[2] * bone_transforms[bone_indices[2]]
        + bone_weights[3] * bone_transforms[bone_indices[3]]
        + (1.0f - sum) * mat4(1.0f);

    mat4 animation_matrix = model_matrix * bone_transform;
    mat3 normal_matrix = cofactor(animation_matrix);

    vec3 ws_pos = (animation_matrix * vec4(vertex_position, 1.0f)).xyz;
    vec3 ws_norm = normalize(normal_matrix * normal);

    vec3 ws_vdir = normalize(ws_view_position - ws_pos);
    
    //Task F
    vertex_out.texture_coordinate = texture_coordinate * texture_scale;
    vertex_out.ws_position = ws_pos;
    vertex_out.ws_normal = ws_norm;
    vertex_out.ws_view_dir = ws_vdir;
    gl_Position = projection_view_matrix * vec4(ws_pos, 1.0f);
}
