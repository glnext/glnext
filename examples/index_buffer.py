import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()

renderer = instance.renderer((512, 512))

pipeline = renderer.pipeline(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (location = 0) in vec2 in_vert;
        layout (location = 1) in vec4 in_color;

        layout (location = 0) out vec4 out_color;

        void main() {
            gl_Position = vec4(in_vert, 0.0, 1.0);
            out_color = in_color;
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
    vertex_format='2f 4f',
    vertex_count=4,
    index_count=6,
)

pipeline.update(
    vertex_buffer=glnext.pack([
        -0.5, -0.5, 1.0, 0.0, 0.0, 1.0,
        0.5, -0.5, 0.0, 1.0, 0.0, 1.0,
        -0.5, 0.5, 0.0, 0.0, 1.0, 1.0,
        0.5, 0.5, 1.0, 1.0, 1.0, 1.0,
    ]),
    index_buffer=glnext.pack([
        0, 1, 2,
        2, 1, 3,
    ]),
)

instance.render()
data = renderer.output[0].read()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
