import glnext
import glwindow
from glnext_compiler import glsl
from PIL import Image, ImageDraw

instance = glnext.instance(surface=True)
task = instance.task()

img = Image.new('RGBA', (512, 512), '#000')
ImageDraw.Draw(img).text((256, 256), 'Loading...')

loading = instance.image((512, 512), mode='output')
loading.write(img.tobytes())

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

wnd = glwindow.window((512, 512))
wnd.show(True)

frame = 0
surface = instance.surface(wnd.handle, loading)

while wnd.visible:
    glwindow.update()

    frame += 1
    if frame > 120:
        surface.image = framebuffer.output[0]
        task.run()

    instance.present()
