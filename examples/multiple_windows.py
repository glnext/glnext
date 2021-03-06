import glnext
import glwindow
from glnext_compiler import glsl

instance = glnext.instance(surface=True)
task = instance.task()

framebuffer = task.framebuffer((512, 512))

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (location = 0) out vec4 out_color;

        vec2 positions[3] = vec2[](
            vec2(-0.5, -0.5),
            vec2(0.5, -0.5),
            vec2(0.0, 0.5)
        );

        vec4 colors[3] = vec4[](
            vec4(1.0, 0.0, 0.0, 1.0),
            vec4(0.0, 1.0, 0.0, 1.0),
            vec4(0.0, 0.0, 1.0, 1.0)
        );

        void main() {
            gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
            out_color = colors[gl_VertexIndex];
            gl_Position.y *= -1.0;
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

wnd_1 = glwindow.window((512, 512), 'Window 1')
wnd_2 = glwindow.window((512, 512), 'Window 2')

wnd_1.show(True)
wnd_2.show(True)

instance.surface(wnd_1.handle, framebuffer.output[0])
instance.surface(wnd_2.handle, framebuffer.output[0])

while wnd_1.visible or wnd_2.visible:
    glwindow.update()
    task.run()
    instance.present()
