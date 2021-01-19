import os
import pytest


def test_image_invalid_read(instance):
    image = instance.image((4, 4), mode='texture')
    with pytest.raises(ValueError):
        image.read()


def test_image_read_write(instance):
    data = os.urandom(64)
    image = instance.image((4, 4), mode='output')
    image.write(data)
    assert image.read() == data


def test_image_layers(instance):
    data = os.urandom(64 * 3)
    image = instance.image((4, 4), layers=3, mode='output')
    image.write(data)
    assert image.read() == data


def test_image_write_overflow(instance):
    data = os.urandom(1024)
    image = instance.image((4, 4), mode='output')
    with pytest.raises(ValueError):
        image.write(data)


def test_image_write_underflow(instance):
    image = instance.image((4, 4), mode='output')
    with pytest.raises(ValueError):
        image.write(b'')


def test_image_write_type_error(instance):
    image = instance.image((4, 4), mode='output')
    with pytest.raises(TypeError):
        image.write('0' * 64)


def test_image_mipmaps_invalid_output_mode(instance):
    with pytest.raises(ValueError):
        instance.image((4, 4), levels=4, mode='output')


def test_image_mipmaps(instance):
    data = os.urandom(64)
    image = instance.image((4, 4), levels=4, mode='texture')
    image.write(data)
