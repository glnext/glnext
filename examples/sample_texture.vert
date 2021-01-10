#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec2 in_vert;
layout (location = 1) in vec2 in_text;

layout (location = 0) out vec2 out_text;

void main() {
    gl_Position = vec4(in_vert, 0.0, 1.0);
    out_text = in_text;
}
