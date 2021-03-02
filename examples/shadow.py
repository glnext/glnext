import glnext
from glnext_compiler import glsl
from objloader import Obj
from PIL import Image

instance = glnext.instance()

vertex_size = 24
mesh = Obj.open('examples/monkey.obj').pack('vx vy vz nx ny nz')
vertex_count = len(mesh) // vertex_size

uniform_buffer = instance.buffer('uniform_buffer', 160)

vertex_buffer = instance.buffer('vertex_buffer', len(mesh))
vertex_buffer.write(mesh)

shadow_fbo = instance.framebuffer((512, 512), '1f', samples=1, mode='texture')

shadow_fbo.update(
    clear_values=glnext.pack([1.0, 0.0, 0.0, 0.0]),
    clear_depth=1.0,
)

shadow_pipeline = shadow_fbo.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (binding = 0) uniform Buffer {
            mat4 mvp_camera;
            mat4 mvp_light;
            vec3 camera;
            vec3 light;
        };

        layout (location = 0) in vec3 in_vert;
        layout (location = 0) out float out_depth;

        void main() {
            gl_Position = mvp_light * vec4(in_vert, 1.0);
            out_depth = gl_Position.z / gl_Position.w;
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
    vertex_format='3f 12x',
    vertex_count=vertex_count,
    vertex_buffer=vertex_buffer,
    bindings=[
        {
            'binding': 0,
            'type': 'uniform_buffer',
            'buffer': uniform_buffer,
        },
    ],
)

framebuffer = instance.framebuffer((512, 512))

framebuffer.update(
    clear_values=glnext.pack([0.0, 0.0, 0.0, 1.0]),
)

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (binding = 0) uniform Buffer {
            mat4 mvp_camera;
            mat4 mvp_light;
            vec3 camera;
            vec3 light;
        };

        layout (location = 0) in vec3 in_vert;
        layout (location = 1) in vec3 in_norm;

        layout (location = 0) out vec3 out_vert;
        layout (location = 1) out vec3 out_norm;

        void main() {
            gl_Position = mvp_camera * vec4(in_vert, 1.0);
            out_vert = in_vert;
            out_norm = in_norm;
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (binding = 0) uniform Buffer {
            mat4 mvp_camera;
            mat4 mvp_light;
            vec3 camera;
            vec3 light;
        };

        layout (binding = 1) uniform sampler2D Texture;

        layout (location = 0) in vec3 in_vert;
        layout (location = 1) in vec3 in_norm;

        layout (location = 0) out vec4 out_color;

        void main() {
            vec4 temp = mvp_light * vec4(in_vert, 1.0);
            float d1 = texture(Texture, temp.xy * 0.5 + 0.5).r;
            float d2 = temp.z / temp.w;
            float lum = abs(dot(normalize(light - in_vert), normalize(in_norm))) * 0.9 + 0.1;
            if (d1 + 0.005 < d2) {
                lum = 0.1;
            }
            lum = lum * 0.8 + abs(dot(normalize(camera - in_vert), normalize(in_norm))) * 0.2;
            out_color = vec4(lum, lum, lum, 1.0);
        }
    '''),
    vertex_format='3f 3f',
    vertex_count=vertex_count,
    vertex_buffer=vertex_buffer,
    bindings=[
        {
            'binding': 0,
            'type': 'uniform_buffer',
            'buffer': uniform_buffer,
        },
        {
            'binding': 1,
            'type': 'sampled_image',
            'images': [
                {
                    'image': shadow_fbo.output[0],
                    'sampler': {},
                }
            ]
        },
    ],
)

uniform_buffer.write(b''.join([
    glnext.camera((4.0, 3.0, 2.0), (0.0, 0.0, 0.0)),
    glnext.camera((4.0, 3.0, 12.0), (0.0, 0.0, 0.0), fov=0.0, size=3.0, near=1.0, far=16.0),
    glnext.pack([4.0, 3.0, 2.0, 0.0]),
    glnext.pack([4.0, 3.0, 12.0, 0.0]),
]))

instance.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (512, 512), data, 'raw', 'RGBA', 0, -1).show()
