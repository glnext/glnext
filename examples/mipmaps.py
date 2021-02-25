import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()

framebuffer = instance.framebuffer((512, 512))

texture = Image.open('examples/grass.jpg').convert('RGBA')
image = instance.image(texture.size, levels=8)
image.write(texture.tobytes())

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (binding = 0) uniform Buffer {
            mat4 mvp;
        };

        layout (location = 0) in vec3 in_vert;
        layout (location = 1) in vec3 in_text;

        layout (location = 0) out vec3 out_text;

        void main() {
            gl_Position = mvp * vec4(in_vert, 1.0);
            out_text = in_text;
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (binding = 1) uniform sampler2D Texture[];

        layout (location = 0) in vec2 in_text;

        layout (location = 0) out vec4 out_color;

        void main() {
            out_color = texture(Texture[0], in_text);
        }
    '''),
    vertex_format='3f 2f',
    vertex_count=4,
    topology='triangle_strip',
    bindings=[
        {
            'binding': 0,
            'name': 'uniform_buffer',
            'type': 'uniform_buffer',
            'size': 64,
        },
        {
            'binding': 1,
            'type': 'sampled_image',
            'images': [
                {
                    'image': image,
                    'sampler': {
                        'min_filter': 'linear',
                        'mag_filter': 'linear',
                        'mipmap_filter': 'linear',
                    },
                }
            ],
        },
    ],
)

pipeline.update(
    uniform_buffer=glnext.camera((1.0, 1.0, 1.0), (2.0, 2.0, 1.0)),
    vertex_buffer=glnext.pack([
        0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 100.0, 0.0, 0.0, 20.0,
        100.0, 0.0, 0.0, 20.0, 0.0,
        100.0, 100.0, 0.0, 20.0, 20.0,
    ]),
)

instance.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (512, 512), data, 'raw', 'RGBA', 0, -1).show()
