import os
import pytest
import numpy as np


def test_staged_image_write(instance):
    data = os.urandom(64)
    image = instance.image((4, 4), mode='output')
    instance.staging_buffer([
        [image],
    ])
    with pytest.raises(ValueError):
        image.write(data)


def test_staged_image_read(instance):
    data = os.urandom(64)
    image = instance.image((4, 4), mode='output')
    buffer = instance.staging_buffer([
        [image],
    ])
    buffer.mem[:] = data
    instance.render()
    assert image.read() == data


def test_staged_image_packed(instance):
    data = os.urandom(128)
    image1 = instance.image((4, 4), mode='output')
    image2 = instance.image((4, 4), mode='output')
    buffer = instance.staging_buffer([
        [image1, image2],
    ])
    buffer.mem[:] = data
    instance.render()
    assert image1.read() == data[:64]
    assert image2.read() == data[64:]


def test_staged_image_union(instance):
    data = os.urandom(64)
    image1 = instance.image((4, 4), mode='output')
    image2 = instance.image((4, 4), mode='output')
    buffer = instance.staging_buffer([
        [image1],
        [image2],
    ])
    buffer.mem[:] = data
    instance.render()
    assert image1.read() == data
    assert image2.read() == data


def test_staged_transform_input_output(instance, simple_transform):
    buffer = instance.staging_buffer([
        [simple_transform.storage_buffer],
        [simple_transform.output],
    ])
    array = np.frombuffer(buffer.mem, 'f4')
    array[:] = [1.0, 2.0, 3.0, 4.0]
    instance.render()
    np.testing.assert_array_almost_equal(array, [1.5, 2.0, 2.5, 3.0])
    instance.render()
    np.testing.assert_array_almost_equal(array, [1.75, 2.0, 2.25, 2.5])
