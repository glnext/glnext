#version 450
#pragma shader_stage(vertex)

layout (binding = 0) uniform Buffer {
    mat4 mvp;
};

layout (location = 0) in vec3 in_vert;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec4 in_color;

layout (location = 0) out vec3 out_norm;
layout (location = 1) out vec4 out_color;

void main() {
    gl_Position = mvp * vec4(in_vert, 1.0);
    out_norm = in_norm;
    out_color = in_color;
}
