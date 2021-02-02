import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()

texture = Image.open('examples/rock.jpg').convert('RGBA')
image = instance.image(texture.size)
image.write(texture.tobytes())

framebuffer = instance.framebuffer((512, 512))

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (location = 0) in vec2 in_vert;
        layout (location = 1) in vec2 in_text;

        layout (location = 0) out vec2 out_text;

        void main() {
            gl_Position = vec4(in_vert, 0.0, 1.0);
            out_text = in_text;
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (binding = 0) uniform sampler2D Texture[];

        layout (location = 0) in vec2 in_text;
        layout (location = 0) out vec4 out_color;

        void main() {
            out_color = texture(Texture[0], in_text);
        }
    '''),
    vertex_format='2f 2f',
    vertex_count=3,
    images=[
        {
            'binding': 0,
            'type': 'sampled_image',
            'images': [
                {
                    'image': image,
                    'sampler': {
                        'min_filter': 'linear',
                        'mag_filter': 'linear',
                    },
                }
            ],
        },
    ],
)

pipeline.update(
    vertex_buffer=glnext.pack([
        -0.5, -0.5, 0.0, 0.0,
        0.5, -0.5, 1.0, 0.0,
        0.0, 0.5, 0.5, 1.0,
    ]),
)

instance.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
