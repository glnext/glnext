import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance(layers=['VK_LAYER_KHRONOS_validation'])

compute = instance.compute_set(
    compute_shader=glsl('''
        #version 450
        #pragma shader_stage(compute)

        layout (local_size_x = 16, local_size_y = 16) in;
        layout (binding = 0, rgba8) uniform image2D Result;

        void main() {
            vec2 pos = vec2(gl_GlobalInvocationID.xy) / 400.0f;
            vec3 color = vec3(sin(pos.x), sin(pos.y), cos(pos.x + pos.y));
            imageStore(Result, ivec2(gl_GlobalInvocationID.xy), vec4(color, 1.0));
        }
    '''),
    size=(512, 512),
)

instance.render()
data = compute.output[0].read()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'RGBX', 0, -1).show()
