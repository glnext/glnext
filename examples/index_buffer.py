import glnext
from PIL import Image

from utils.shaders import compile_shader

instance = glnext.instance()

ctx = instance.context((512, 512))

renderer = ctx.renderer(
    vertex_shader=compile_shader('examples/color_shapes.vert'),
    fragment_shader=compile_shader('examples/color_passthrough.frag'),
    vertex_format='2f 4f',
    vertex_count=4,
    index_count=6,
)

renderer.update(
    vertex_buffer=glnext.pack([
        -0.5, -0.5, 1.0, 0.0, 0.0, 1.0,
        0.5, -0.5, 0.0, 1.0, 0.0, 1.0,
        -0.5, 0.5, 0.0, 0.0, 1.0, 1.0,
        0.5, 0.5, 1.0, 1.0, 1.0, 1.0,
    ]),
    index_buffer=glnext.pack([
        0, 1, 2,
        2, 1, 3,
    ]),
)

data = ctx.render()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
