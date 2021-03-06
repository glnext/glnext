import time

import glnext
from glnext_compiler import glsl

vertex_shader = glsl('''
    #version 450
    #pragma shader_stage(vertex)

    layout (location = 0) out vec4 out_color;

    vec2 positions[3] = vec2[](
        vec2(-0.5, -0.5),
        vec2(0.5, -0.5),
        vec2(0.0, 0.5)
    );

    vec4 colors[3] = vec4[](
        vec4(1.0, 0.0, 0.0, 1.0),
        vec4(0.0, 1.0, 0.0, 1.0),
        vec4(0.0, 0.0, 1.0, 1.0)
    );

    void main() {
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
        out_color = colors[gl_VertexIndex];
    }
''')

fragment_shader = glsl('''
    #version 450
    #pragma shader_stage(fragment)

    layout (location = 0) in vec4 in_color;
    layout (location = 0) out vec4 out_color;

    void main() {
        out_color = in_color;
    }
''')


def create_pipeline(instance):
    task = instance.task()
    framebuffer = task.framebuffer((512, 512))
    framebuffer.render(
        vertex_shader=vertex_shader,
        fragment_shader=fragment_shader,
        vertex_count=3,
    )


cache = b''
instance1 = glnext.instance(cache=cache)

a = time.perf_counter_ns()
create_pipeline(instance1)
b = time.perf_counter_ns()

cache = instance1.cache()
instance2 = glnext.instance(cache=cache)

c = time.perf_counter_ns()
create_pipeline(instance2)
d = time.perf_counter_ns()

print('cache size:', len(cache))
print('without cache:', (b - a) / 1e6)
print('with cache:', (d - c) / 1e6)
