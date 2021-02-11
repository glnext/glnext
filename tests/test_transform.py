from glnext_compiler import glsl
import pytest
import numpy as np


def test_transform_input_output(instance):
    transfrom = instance.compute(
        compute_count=8,
        compute_shader=glsl('''
            #version 450
            #pragma shader_stage(compute)

            layout (binding = 0) buffer Input {
                int number[];
            };

            layout (binding = 1) buffer Output {
                int result[];
            };

            void main() {
                result[gl_GlobalInvocationID.x] = number[gl_GlobalInvocationID.x] + 1;
            }
        '''),
        bindings=[
            {
                'binding': 0,
                'name': 'input_buffer',
                'type': 'input_buffer',
                'size': 32,
            },
            {
                'binding': 1,
                'name': 'output_buffer',
                'type': 'output_buffer',
                'size': 32,
            },
        ]
    )

    data = np.array([1, 2, 3, 4, 5, 6, 7, 8], 'u4')
    transfrom['input_buffer'].write(data)
    instance.run()
    array = np.frombuffer(transfrom['output_buffer'].read(), 'u4')
    np.testing.assert_array_almost_equal(array, data + 1)
