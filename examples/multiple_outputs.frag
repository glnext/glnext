#version 450
#pragma shader_stage(fragment)

layout (location = 0) out vec4 out_color_1;
layout (location = 1) out vec4 out_color_2;

void main() {
    out_color_1 = vec4(0.8, 0.4, 0.1, 1.0);
    out_color_2 = vec4(0.1, 0.4, 0.8, 1.0);
}
