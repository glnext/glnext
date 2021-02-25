import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()

framebuffer = instance.framebuffer((512, 512))

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (location = 0) in vec2 in_vert;
        layout (location = 1) in vec3 in_color;

        layout (location = 0) out vec3 out_color;

        void main() {
            gl_Position = vec4(in_vert, 0.0, 1.0);
            out_color = in_color;
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (location = 0) in vec3 in_color;
        layout (location = 0) out vec4 out_color;

        void main() {
            out_color = vec4(in_color, 1.0);
        }
    '''),
    vertex_format='2f 3f',
    vertex_count=3,
)

pipeline['vertex_buffer'].write(glnext.pack([
    -0.5, -0.5, 0.0, 0.0, 1.0,
    0.5, -0.5, 0.0, 1.0, 0.0,
    0.0, 0.5, 1.0, 0.0, 0.0,
]))

instance.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (512, 512), data, 'raw', 'RGBA', 0, -1).show()
