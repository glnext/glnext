import glnext
from PIL import Image

from utils.shaders import compile_shader

instance = glnext.instance()

ctx = instance.context((512, 512), '4b 4b', samples=4)

renderer = ctx.renderer(
    vertex_shader=compile_shader('examples/multiple_outputs.vert'),
    fragment_shader=compile_shader('examples/multiple_outputs.frag'),
    vertex_count=3,
)

data1, data2 = ctx.render()

Image.frombuffer('RGB', (512, 512), data1, 'raw', 'BGRX', 0, -1).show()
Image.frombuffer('RGB', (512, 512), data2, 'raw', 'BGRX', 0, -1).show()
