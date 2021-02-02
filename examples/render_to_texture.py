import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()

triangle_framebuffer = instance.framebuffer((512, 512), mode='texture')

triangle_pipeline = triangle_framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (location = 0) out vec4 out_color;

        vec2 positions[3] = vec2[](
            vec2(-0.9, -0.9),
            vec2(0.9, -0.9),
            vec2(0.0, 0.9)
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
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (location = 0) in vec4 in_color;
        layout (location = 0) out vec4 out_color;

        void main() {
            out_color = in_color;
        }
    '''),
    vertex_count=3,
)

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
                    'image': triangle_framebuffer.output[0],
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
        0.5, -0.5, 10.0, 0.0,
        0.0, 0.5, 5.0, 10.0,
    ]),
)

instance.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
