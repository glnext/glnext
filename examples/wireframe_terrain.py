import glnext
import numpy as np
from glnext_compiler import glsl
from PIL import Image


def terrain(size):
    V = np.random.normal(0.0, 0.005, size * size)

    for _ in range(128):
        k = np.random.randint(5, 15)
        x, y = np.random.randint(0, size, 2)
        sx, ex, sy, ey = np.clip([x - k, x + k + 1, y - k, y + k + 1], 0, size)
        IX, IY = np.meshgrid(np.arange(sx, ex), np.arange(sy, ey))
        D = np.clip(np.sqrt(np.square((IX - x) / k) + np.square((IY - y) / k)), 0.0, 1.0)
        V[IX * size + IY] += np.cos(D * np.pi / 2.0) * k * np.random.normal(0.0, 0.005)

    X, Y = np.meshgrid(np.linspace(-1.0, 1.0, size), np.linspace(-1.0, 1.0, size))
    P = np.array([X.flatten(), Y.flatten(), V]).T
    A, B = np.meshgrid(np.arange(size + 1), np.arange(size))
    I = np.concatenate([A + B * size, A * size + B])
    I[:, -1] = -1
    return P.astype('f4').tobytes(), I.astype('i4').tobytes()


vertex_data, index_data = terrain(64)

instance = glnext.instance()
task = instance.task()

framebuffer = task.framebuffer((1280, 720))

framebuffer.update(
    clear_values=glnext.pack([1.0, 1.0, 1.0, 1.0]),
)

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
    vertex_format='3f',
    vertex_count=len(vertex_data) // 12,
    index_count=len(index_data) // 4,
    topology='line_strip',
    bindings=[
        {
            'binding': 0,
            'name': 'uniform_buffer',
            'type': 'uniform_buffer',
            'size': 64,
        },
    ],
)

pipeline.update(
    uniform_buffer=glnext.camera((2.0, 2.0, 1.0), (0.0, 0.0, 0.0), fov=45.0, aspect=16 / 9),
    vertex_buffer=vertex_data,
    index_buffer=index_data,
)

task.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (1280, 720), data, 'raw', 'RGBA', 0, -1).show()
