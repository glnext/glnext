from glnext_compiler import glsl
import pytest
import numpy as np


def test_simple_transform(instance, simple_transform):
    transfrom = instance.transform(
        storage_buffer=32,
        output_buffer=32,
        compute_groups=(8, 1, 1),
        compute_shader=glsl('''
            #version 450
            #pragma shader_stage(compute)

            layout (binding = 0) buffer StorageBuffer {
                int number[];
            };

            layout (binding = 1) buffer Output {
                int result[];
            };

            void main() {
                result[gl_GlobalInvocationID.x] = number[gl_GlobalInvocationID.x] + 1;
            }
        '''),
    )

    data = np.array([1, 2, 3, 4, 5, 6, 7, 8], 'u4')
    transfrom.storage_buffer.write(data)
    instance.render()
    array = np.frombuffer(transfrom.output.read(), 'u4')
    np.testing.assert_array_almost_equal(array, data + 1)
