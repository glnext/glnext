import glnext
import numpy as np
from glnext_compiler import glsl
from matplotlib import pyplot as plt
from objloader import Obj

instance = glnext.instance()
task = instance.task()

framebuffer = task.framebuffer((512, 512), '1f', samples=1)

vertex_size = 12
mesh = Obj.open('examples/monkey.obj').pack('vx vy vz')
vertex_count = len(mesh) // vertex_size

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (binding = 0) uniform Buffer {
            mat4 mvp;
        };

        layout (location = 0) in vec3 in_vert;

        layout (location = 0) out float out_depth;

        void main() {
            vec4 vert = mvp * vec4(in_vert, 1.0);
            gl_Position = vert;
            out_depth = vert.z / vert.w;
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (location = 0) in float in_depth;

        layout (location = 0) out float out_depth;

        void main() {
            out_depth = in_depth;
        }
    '''),
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
    clear_values=glnext.pack([1.0, 0.0, 0.0, 0.0]),
    clear_depth=1.0,
)

pipeline.update(
    uniform_buffer=glnext.camera((4.0, 3.0, 2.0), (0.0, 0.0, 0.0), near=1.0, far=7.0),
    vertex_buffer=mesh,
)

task.run()
data = framebuffer.output[0].read()
array = np.ndarray((512, 512), 'f4', data)[::-1]
plt.imshow(array, 'YlGnBu', vmin=0.8, vmax=1.0)
plt.show()
