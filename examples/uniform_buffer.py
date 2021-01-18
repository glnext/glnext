import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()

renderer = instance.renderer((512, 512))

pipeline = renderer.pipeline(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (binding = 0) uniform Buffer {
            vec2 scale;
        };

        layout (location = 0) out vec3 out_color;

        vec2 positions[3] = vec2[](
            vec2(-0.5, -0.5),
            vec2(0.5, -0.5),
            vec2(0.0, 0.5)
        );

        vec3 colors[3] = vec3[](
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, 1.0, 0.0),
            vec3(0.0, 0.0, 1.0)
        );

        void main() {
            gl_Position = vec4(positions[gl_VertexIndex] * scale, 0.0, 1.0);
            out_color = colors[gl_VertexIndex];
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
    vertex_count=3,
)

renderer.update(
    uniform_buffer=glnext.pack([0.3, 1.5]),
)

instance.render()
data = renderer.output[0].read()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
