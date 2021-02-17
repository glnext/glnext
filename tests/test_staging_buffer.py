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
    assert len(staging.mem) == 64 + 4
    staging.mem[:4] = glnext.pack([1])
    staging.mem[4:] = data
    instance.run()
    assert image.read() == data
    staging.mem[:4] = glnext.pack([0])
    staging.mem[4:] = os.urandom(64)
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
    assert len(staging.mem) == 64 + 4
    staging.mem[:4] = glnext.pack([64])
    staging.mem[4:] = data
    instance.run()
    assert buffer.read() == data
    staging.mem[:4] = glnext.pack([0])
    staging.mem[4:] = os.urandom(64)
    instance.run()
    assert buffer.read() == data


def test_staging_input_size(instance):
    data1 = os.urandom(32)
    data2 = os.urandom(32)
    data3 = os.urandom(32)
    buffer = instance.buffer('storage_buffer', 64, readable=True)
    staging = instance.staging([
        {
            'offset': 0,
            'type': 'input_buffer',
            'buffer': buffer,
        }
    ])
    staging.mem[:4] = glnext.pack([64])
    staging.mem[4:] = data1 + data2
    instance.run()
    assert buffer.read() == data1 + data2
    staging.mem[:4] = glnext.pack([32])
    staging.mem[4:] = data3 + os.urandom(32)
    instance.run()
    assert buffer.read() == data3 + data2
