import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()

framebuffer = instance.framebuffer((512, 512), '4p 4p', samples=4)

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        vec2 positions[3] = vec2[](
            vec2(-0.5, -0.5),
            vec2(0.5, -0.5),
            vec2(0.0, 0.5)
        );

        void main() {
            gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (location = 0) out vec4 out_color_1;
        layout (location = 1) out vec4 out_color_2;

        void main() {
            out_color_1 = vec4(0.8, 0.4, 0.1, 1.0);
            out_color_2 = vec4(0.1, 0.4, 0.8, 1.0);
        }
    '''),
    vertex_count=3,
)

instance.run()
data1 = framebuffer.output[0].read()
data2 = framebuffer.output[1].read()

Image.frombuffer('RGBA', (512, 512), data1, 'raw', 'RGBA', 0, -1).show()
Image.frombuffer('RGBA', (512, 512), data2, 'raw', 'RGBA', 0, -1).show()
