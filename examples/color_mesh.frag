#version 450
#pragma shader_stage(fragment)

layout (binding = 0) uniform Buffer {
    mat4 mvp;
};

layout (location = 0) in vec3 in_norm;
layout (location = 1) in vec4 in_color;

layout (location = 0) out vec4 out_color;

void main() {
    vec3 color = vec3(1.0, 1.0, 1.0);
    vec3 sight = -vec3(mvp[0].w, mvp[1].w, mvp[2].w);
    float lum = dot(normalize(sight), normalize(in_norm));
    out_color = vec4(in_color.rgb * (lum * 0.7 + 0.3), in_color.a);
}
