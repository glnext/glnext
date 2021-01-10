#version 450
#pragma shader_stage(fragment)

layout (binding = 1) uniform sampler2D Texture[];

layout (location = 0) in vec2 in_text;
layout (location = 0) out vec4 out_color;

void main() {
    out_color = texture(Texture[0], in_text);
}
