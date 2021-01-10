import glnext
from PIL import Image

from utils.mesh import white_cube
from utils.shaders import compile_shader

instance = glnext.instance()

ctx = instance.context((512, 512))

ctx.update(uniform_buffer=glnext.camera((4.0, 3.0, 2.0), (0.0, 0.0, 0.0)))

renderer = ctx.renderer(
    vertex_shader=compile_shader('examples/color_mesh.vert'),
    fragment_shader=compile_shader('examples/color_mesh.frag'),
    vertex_format='3f 3f 4f',
    vertex_count=36,
)

renderer.update(vertex_buffer=white_cube)

data = ctx.render()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
