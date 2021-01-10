import json

import glnext
from PIL import Image

from utils.shaders import compile_shader

instance = glnext.instance()

ctx1 = instance.context((256, 256), samples=1)
ctx2 = instance.context((512, 512), '4b 4b', samples=4)

image = instance.image((256, 256))

renderer = ctx2.renderer(
    vertex_shader=compile_shader('examples/color_mesh.vert'),
    fragment_shader=compile_shader('examples/color_mesh.frag'),
    vertex_format='3f 3f 4f',
    instance_format='4f',
    vertex_count=10,
    instance_count=10,
    index_count=10,
    indirect_count=10,
    samplers=[instance.sampler(image)],
)

print(json.dumps(instance.info(), indent=2))
instance.release()
