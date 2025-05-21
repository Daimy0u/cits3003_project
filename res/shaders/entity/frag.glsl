#version 410 core
#include "../common/lights.glsl"

in VertexOut {
    vec3 ws_position;
    vec3 ws_normal;
    vec3 ws_view_dir;
    vec2 texture_coordinate;
} frag_in;

layout(location = 0) out vec4 out_colour;

// Global Data
uniform float inverse_gamma;
uniform sampler2D diffuse_texture;
uniform sampler2D specular_map_texture;

uniform vec3 diffuse_tint;
uniform vec3 specular_tint;
uniform vec3 ambient_tint;
uniform float shininess;

#if NUM_PL > 0
layout(std140) uniform PointLightArray {
    PointLightData point_lights[NUM_PL];
};
#endif
#if NUM_DL > 0
layout(std140) uniform DirectionalLightArray {
    DirectionalLightData directional_lights[NUM_DL];
};
#endif

void main() {
    LightCalculatioData data = LightCalculatioData(
        frag_in.ws_position,
        frag_in.ws_view_dir,
        normalize(frag_in.ws_normal)
    );

    Material material = Material(
        diffuse_tint,
        specular_tint,
        ambient_tint,
        shininess
    );

    LightingResult lighting = total_light_calculation(data, material
        #if NUM_PL > 0
        , point_lights
        #endif
        #if NUM_DL > 0
        , directional_lights
        #endif
    );

    vec3 result = resolve_textured_light_calculation(lighting, diffuse_texture, specular_map_texture, frag_in.texture_coordinate);
    out_colour = vec4(pow(result, vec3(inverse_gamma)), 1.0);
}
