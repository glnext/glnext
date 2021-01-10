import glnext
from PIL import Image

from utils.shaders import compile_shader

instance = glnext.instance()

ctx = instance.context((512, 512))

renderer = ctx.renderer(
    vertex_shader=compile_shader('examples/uniform_buffer.vert'),
    fragment_shader=compile_shader('examples/uniform_buffer.frag'),
    vertex_count=3,
)

ctx.update(
    uniform_buffer=glnext.pack([0.3, 1.5]),
)

data = ctx.render()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
