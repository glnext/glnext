#version 450
#pragma shader_stage(vertex)

layout (location = 0) out vec4 out_color;

vec2 positions[3] = vec2[](
    vec2(-0.3, -0.3),
    vec2(0.3, -0.3),
    vec2(0.0, 0.3)
);

vec4 colors[3] = vec4[](
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0),
    vec4(0.0, 0.0, 1.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    gl_Position.xy += vec2(0.15, 0.1) * (gl_InstanceIndex - 4);
    out_color = colors[gl_VertexIndex];
}
