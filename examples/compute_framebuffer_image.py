import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()
task = instance.task()

framebuffer = task.framebuffer((512, 512), compute=True)

compute = framebuffer.compute(
    compute_count=(512, 512),
    compute_shader=glsl('''
        #version 450
        #pragma shader_stage(compute)

        layout (binding = 0, rgba8) uniform image2D Result;

        void main() {
            vec2 pos = vec2(gl_GlobalInvocationID.xy) / 400.0f;
            vec3 color = vec3(sin(pos.x), sin(pos.y), cos(pos.x + pos.y));
            imageStore(Result, ivec2(gl_GlobalInvocationID.xy), vec4(color, 1.0));
        }
    '''),
    bindings=[
        {
            'binding': 0,
            'type': 'storage_image',
            'images': [
                {
                    'image': framebuffer.output[0],
                }
            ],
        },
    ],
)

task.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (512, 512), data, 'raw', 'RGBA', 0, -1).show()
