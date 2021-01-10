import glnext
from PIL import Image

from utils.shaders import compile_shader

instance = glnext.instance()

ctx = instance.context((512, 512))

renderer = ctx.renderer(
    vertex_shader=compile_shader('examples/vertex_buffer.vert'),
    fragment_shader=compile_shader('examples/vertex_buffer.frag'),
    vertex_format='2f 3f',
    vertex_count=3,
)

renderer.update(
    vertex_buffer=glnext.pack([
        -0.5, -0.5, 0.0, 0.0, 1.0,
        0.5, -0.5, 0.0, 1.0, 0.0,
        0.0, 0.5, 1.0, 0.0, 0.0,
    ]),
)

data = ctx.render()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
