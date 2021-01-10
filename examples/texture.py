import glnext
from PIL import Image

from utils.shaders import compile_shader
from utils.textures import noise

instance = glnext.instance()

image = instance.image(noise.size, '4b')
image.write(noise.tobytes())

sampler = instance.sampler(image)

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
        0.5, -0.5, 1.0, 0.0,
        0.0, 0.5, 0.5, 1.0,
    ]),
)

data = ctx.render()
Image.frombuffer('RGB', (512, 512), data, 'raw', 'BGRX', 0, -1).show()
