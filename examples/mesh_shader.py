import glnext
from glnext_compiler import glsl
from PIL import Image

instance = glnext.instance()

framebuffer = instance.framebuffer((512, 512))

pipeline = framebuffer.render(
    mesh_shader=glsl('''
        #version 450
        #pragma shader_stage(mesh)
        #extension GL_NV_mesh_shader : require

        layout (local_size_x = 1) in;
        layout (max_vertices = 4, max_primitives = 2) out;
        layout (triangles) out;

        void main() {
            gl_MeshVerticesNV[0].gl_Position = vec4(-0.5, -0.5, 0.0, 1.0);
            gl_MeshVerticesNV[1].gl_Position = vec4(0.5, -0.5, 0.0, 1.0);
            gl_MeshVerticesNV[2].gl_Position = vec4(-0.5, 0.5, 0.0, 1.0);
            gl_MeshVerticesNV[3].gl_Position = vec4(0.5, 0.5, 0.0, 1.0);
            gl_PrimitiveIndicesNV[0] = 0;
            gl_PrimitiveIndicesNV[1] = 1;
            gl_PrimitiveIndicesNV[2] = 2;
            gl_PrimitiveIndicesNV[3] = 2;
            gl_PrimitiveIndicesNV[4] = 1;
            gl_PrimitiveIndicesNV[5] = 3;
            gl_PrimitiveCountNV += 2;
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (location = 0) out vec4 out_color;

        void main() {
            out_color = vec4(1.0, 1.0, 1.0, 1.0);
        }
    '''),
)

instance.run()
data = framebuffer.output[0].read()
Image.frombuffer('RGBA', (512, 512), data, 'raw', 'RGBA', 0, -1).show()
