import glnext
from glnext_compiler import glsl
import pytest


@pytest.fixture()
def instance():
    instance = glnext.instance(
        headless=True,
        application_name='glnext_tests',
        layers=['VK_LAYER_KHRONOS_validation'],
    )
    yield instance
    # instance.release()


@pytest.fixture()
def simple_transform(instance):
    return instance.transform(
        storage_buffer=16,
        output_buffer=16,
        compute_groups=(4, 1, 1),
        compute_shader=glsl('''
            #version 450
            #pragma shader_stage(compute)

            layout (binding = 0) buffer StorageBuffer {
                float number[];
            };

            layout (binding = 1) buffer Output {
                float result[];
            };

            void main() {
                result[gl_GlobalInvocationID.x] = number[gl_GlobalInvocationID.x] * 0.5 + 1.0;
            }
        '''),
    )
