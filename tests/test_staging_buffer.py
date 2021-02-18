import os
import glnext


def test_staging_input_image(instance):
    data = os.urandom(64)
    image = instance.image((4, 4), mode='output')
    staging = instance.staging([
        {
            'offset': 0,
            'type': 'input_image',
            'image': image,
        }
    ])
    assert len(staging.mem) == 64
    staging.mem[:] = data
    instance.run()
    assert image.read() == data


def test_staging_input_buffer(instance):
    data = os.urandom(64)
    buffer = instance.buffer('storage_buffer', 64, readable=True)
    staging = instance.staging([
        {
            'offset': 0,
            'type': 'input_buffer',
            'buffer': buffer,
        }
    ])
    assert len(staging.mem) == 64
    staging.mem[:] = data
    instance.run()
    assert buffer.read() == data
