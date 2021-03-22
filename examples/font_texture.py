import glnext
from glnext_compiler import glsl
from PIL import Image, ImageDraw, ImageFont

def build_font(symbol, font):
    img = Image.new('RGBA', (16, 16))
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), symbol, '#fff', font)
    return img.tobytes()

font = ImageFont.truetype('consola.ttf', size=20)
data = b''.join(build_font(chr(i), font) for i in range(128))

instance = glnext.instance()
task = instance.task()

image = instance.image((16, 16), layers=128)
image.write(data)

framebuffer = task.framebuffer((512, 512))

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (location = 0) in vec2 in_vert;
        layout (location = 1) in vec2 in_text;
        layout (location = 2) in float in_char;

        layout (location = 0) out vec3 out_text;

        void main() {
            gl_Position = vec4(in_vert, 0.0, 1.0);
            gl_Position.x += 0.1 * gl_InstanceIndex;
            out_text = vec3(in_text, in_char * 255.0);
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (binding = 0) uniform sampler2DArray Texture[];

        layout (location = 0) in vec3 in_text;
        layout (location = 0) out vec4 out_color;

        void main() {
            out_color = texture(Texture[0], in_text);
        }
    '''),
    vertex_format='2f 2f',
    vertex_count=4,
    instance_format='1p',
    instance_count=12,
    topology='triangle_strip',
    bindings=[
        {
            'binding': 0,
            'type': 'sampled_image',
            'images': [
                {
                    'image': image,
                    'array': True,
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
        -0.05, -0.05, 0.0, 1.0,
        -0.05, 0.05, 0.0, 0.0,
        0.05, -0.05, 1.0, 1.0,
        0.05, 0.05, 1.0, 0.0,
    ]),
    instance_buffer=b'Hello World!',
)

task.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (512, 512), data, 'raw', 'RGBA', 0, -1).show()
