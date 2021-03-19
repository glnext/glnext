import glnext
from glnext_compiler import glsl
import pytest


@pytest.fixture()
def instance():
    instance = glnext.instance(
        application_name='glnext_tests',
        debug=True,
    )
    yield instance
    # instance.release()
