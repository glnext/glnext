import os
import pytest
import glnext


@pytest.fixture()
def instance():
    instance = glnext.instance(
        headless=True,
        application_name='glnext_tests',
        layers=os.getenv('TEST_VULKAN_LAYERS', '').split(),
    )
    yield instance
    instance.release()
