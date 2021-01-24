import glnext
import numpy as np


def test_pack_floats():
    array = [1.0, 2.5, 0.3333]
    assert glnext.pack(array) == np.array(array, 'f4').tobytes()


def test_pack_int():
    array = [512, 0, -1]
    assert glnext.pack(array) == np.array(array, 'i4').tobytes()


def test_pack_mixed():
    data = glnext.pack('2f 2i', [1.0, 2.0, 1, 2])
    assert data.hex() == '0000803f000000400100000002000000'


def test_pack_mixed_rows():
    data = glnext.pack('1f 1i', [
        0.2, 10,
        0.7, 20,
    ])
    assert data.hex() == 'cdcc4c3e0a0000003333333f14000000'


def test_pack_padding():
    data = glnext.pack('2x 1f 1i', [
        0.2, 10,
        0.7, 20,
    ])
    assert data.hex() == '0000cdcc4c3e0a00000000003333333f14000000'

    data = glnext.pack('1f 2x 1i', [
        0.2, 10,
        0.7, 20,
    ])
    assert data.hex() == 'cdcc4c3e00000a0000003333333f000014000000'

    data = glnext.pack('1f 1i 2x', [
        0.2, 10,
        0.7, 20,
    ])
    assert data.hex() == 'cdcc4c3e0a00000000003333333f140000000000'
