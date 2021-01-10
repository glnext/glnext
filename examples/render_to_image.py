import glnext
from PIL import Image

from utils.shaders import compile_shader

instance = glnext.instance()

triangle_ctx = instance.context((512, 512), render_to_image=True)

triangle_renderer = triangle_ctx.renderer(
    vertex_shader=compile_shader('examples/hello_triangle.vert'),
    fragment_shader=compile_shader('examples/color_passthrough.frag'),
    vertex_count=3,
)

triangle_ctx.render()

sampler = instance.sampler(triangle_ctx.output[0])

ctx = instance.context((512, 512))

renderer = ctx.renderer(
    vertex_shader=compile_shader('examples/sample_texture.vert'),
    fragment_shader=compile_shader('examples/sample_texture.frag'),
    vertex_format='2f 2f',
    vertex_count=3,
    samplers=[sampler],
)

renderer.update(
    vertex_buffer=glnext.pack([
        -0.5, -0.5, 0.0, 0.0,
        0.5, -0.5, 10.0, 0.0,
        0.0, 0.5, 5.0, 10.0,
    ]),
)

data = ctx.render()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
