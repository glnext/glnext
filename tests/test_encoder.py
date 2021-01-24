import os

from glnext_compiler import glsl
import pytest
import numpy as np


def test_encode_image(instance):
    data = os.urandom(64)
    image = instance.image((4, 4), mode='storage')
    image.write(data)

    encoder = instance.encoder(
        image=image,
        output_buffer=64,
        compute_shader=glsl('''
            #version 450
            #pragma shader_stage(compute)

            layout (binding = 0, rgba8) uniform image2D Input;

            layout (binding = 1) buffer Result {
                float result[];
            };

            void main() {
                vec4 color = imageLoad(Input, ivec2(gl_GlobalInvocationID.xy));
                uint index = gl_GlobalInvocationID.y * 4 + gl_GlobalInvocationID.x;
                result[index] = color.r + color.g + color.b + color.a;
            }
        '''),
    )

    instance.render()

    expected = (np.ndarray((16, 4), 'u1', data) / 255).sum(axis=1)
    array = np.frombuffer(encoder.output.read(), 'f4')
    np.testing.assert_array_almost_equal(array, expected)


def test_encode_image_compatible_format(instance):
    data = os.urandom(64)
    image = instance.image((4, 4), mode='storage')
    image.write(data)

    encoder = instance.encoder(
        image=image,
        format='4b',
        output_buffer=64,
        compute_shader=glsl('''
            #version 450
            #pragma shader_stage(compute)

            layout (binding = 0, rgba8ui) uniform uimage2D Input;

            layout (binding = 1) buffer Result {
                uint result[];
            };

            void main() {
                uvec4 color = imageLoad(Input, ivec2(gl_GlobalInvocationID.xy));
                uint index = gl_GlobalInvocationID.y * 4 + gl_GlobalInvocationID.x;
                result[index] = color.r + color.g + color.b + color.a;
            }
        '''),
    )

    instance.render()

    expected = (np.ndarray((16, 4), 'u1', data).astype('u4')).sum(axis=1)
    array = np.frombuffer(encoder.output.read(), 'u4')
    np.testing.assert_array_almost_equal(array, expected)
