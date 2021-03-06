import glnext
import numpy as np
from glnext_compiler import glsl
from objloader import Obj
from PIL import Image

A = np.linspace(-1.0, 1.0, 11)
B = np.ones(11)
C = np.zeros(11)

vertex_data = np.array([A, -B, C, A, B, C, -B, A, C, B, A, C]).T.astype('f4').tobytes()
vertex_count = len(vertex_data) // 12

instance = glnext.instance()
task = instance.task()

framebuffer = task.framebuffer((512, 512))

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (binding = 0) uniform Buffer {
            mat4 mvp;
        };

        layout (location = 0) in vec3 in_vert;

        void main() {
            gl_Position = mvp * vec4(in_vert, 1.0);
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (location = 0) out vec4 out_color;

        void main() {
            out_color = vec4(0.0, 0.0, 0.0, 1.0);
        }
    '''),
    topology='lines',
    vertex_format='3f',
    vertex_count=vertex_count,
    bindings=[
        {
            'binding': 0,
            'name': 'uniform_buffer',
            'type': 'uniform_buffer',
            'size': 64,
        },
    ],
)

framebuffer.update(
    clear_values=glnext.pack([1.0, 1.0, 1.0, 1.0]),
)

pipeline.update(
    uniform_buffer=glnext.camera((3.0, 2.0, 2.0), (0.0, 0.0, 0.0)),
    vertex_buffer=vertex_data,
)

task.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (512, 512), data, 'raw', 'RGBA', 0, -1).show()
