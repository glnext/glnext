import pytest
import glnext


@pytest.fixture()
def instance():
    instance = glnext.instance(
        headless=True,
        application_name='glnext_tests',
        layers=['VK_LAYER_KHRONOS_validation'],
    )
    yield instance
    instance.release()
