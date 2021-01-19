import pytest
import glnext


@pytest.fixture()
def instance():
    instance = glnext.instance(
        headless=True,
        application_name='glnext_tests',
        layers=[
            'VK_LAYER_LUNARG_core_validation',
            'VK_LAYER_LUNARG_parameter_validation',
            'VK_LAYER_LUNARG_standard_validation',
        ],
    )
    yield instance
    instance.release()
