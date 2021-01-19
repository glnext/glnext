import os
import glnext
import pytest

layers = os.getenv('TEST_VULKAN_LAYERS', '').split()


def test_instance_simple():
    instance = glnext.instance()
    instance.release()


def test_instance_host_memory():
    instance = glnext.instance(host_memory=128 * 1024, headless=True, layers=layers)
    assert instance.device_memory is None
    assert instance.staging_buffer is None
    instance.release()


def test_instance_device_memory():
    instance = glnext.instance(device_memory=1024, headless=True, layers=layers)
    assert instance.device_memory is not None
    assert instance.staging_buffer is None
    instance.release()


def test_instance_staging_buffer():
    instance = glnext.instance(staging_buffer=4096, headless=True, layers=layers)
    assert instance.device_memory is None
    assert type(instance.staging_buffer) is memoryview
    assert len(instance.staging_buffer) == 4096
    instance.release()


def test_instance_invalid_layer():
    with pytest.raises(RuntimeError):
        glnext.instance(layers=['Hello World!'])


def test_instance_invalid_physical_device():
    with pytest.raises(RuntimeError):
        glnext.instance(physical_device=8)
