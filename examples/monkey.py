import glnext
from glnext_compiler import glsl
from objloader import Obj
from PIL import Image

instance = glnext.instance()

framebuffer = instance.framebuffer((512, 512))

vertex_size = 24
mesh = Obj.open('examples/monkey.obj').pack('vx vy vz nx ny nz')
vertex_count = len(mesh) // vertex_size

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (binding = 0) uniform Buffer {
            mat4 mvp;
        };

        layout (location = 0) in vec3 in_vert;
        layout (location = 1) in vec3 in_norm;

        layout (location = 0) out vec3 out_norm;

        void main() {
            gl_Position = mvp * vec4(in_vert, 1.0);
            out_norm = in_norm;
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (binding = 0) uniform Buffer {
            mat4 mvp;
        };

        layout (location = 0) in vec3 in_norm;

        layout (location = 0) out vec4 out_color;

        void main() {
            vec3 light = vec3(4.0, 3.0, 10.0);
            float lum = dot(normalize(light), normalize(in_norm)) * 0.7 + 0.3;
            out_color = vec4(lum, lum, lum, 1.0);
        }
    '''),
    vertex_format='3f 3f',
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

pipeline.update(
    uniform_buffer=glnext.camera((4.0, 3.0, 2.0), (0.0, 0.0, 0.0)),
    vertex_buffer=mesh,
)

instance.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (512, 512), data, 'raw', 'RGBA', 0, -1).show()
