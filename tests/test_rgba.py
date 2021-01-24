import os

import glnext
import numpy as np


def test_rgba_to_rgba():
    data = os.urandom(256)
    result = glnext.rgba(data, 'rgba')
    assert len(result) == 256
    assert result == data


def test_bgra_to_rgba():
    data = os.urandom(256)
    array = np.ndarray((8, 8, 4), 'u1', data)
    result = glnext.rgba(data, 'bgra')
    assert len(result) == 256
    expected = np.zeros((8, 8, 4), 'u1')
    expected[:, :, 0] = array[:, :, 2]
    expected[:, :, 1] = array[:, :, 1]
    expected[:, :, 2] = array[:, :, 0]
    expected[:, :, 3] = array[:, :, 3]
    assert result == expected.tobytes()


def test_rgb_to_rgba():
    data = os.urandom(192)
    array = np.ndarray((8, 8, 3), 'u1', data)
    result = glnext.rgba(data, 'rgb')
    assert len(result) == 256
    expected = np.full((8, 8, 4), 255, 'u1')
    expected[:, :, 0] = array[:, :, 0]
    expected[:, :, 1] = array[:, :, 1]
    expected[:, :, 2] = array[:, :, 2]
    assert result == expected.tobytes()


def test_bgr_to_rgba():
    data = os.urandom(192)
    array = np.ndarray((8, 8, 3), 'u1', data)
    result = glnext.rgba(data, 'bgr')
    assert len(result) == 256
    expected = np.full((8, 8, 4), 255, 'u1')
    expected[:, :, 0] = array[:, :, 2]
    expected[:, :, 1] = array[:, :, 1]
    expected[:, :, 2] = array[:, :, 0]
    assert result == expected.tobytes()


def test_lum_to_rgba():
    data = os.urandom(64)
    array = np.ndarray((8, 8), 'u1', data)
    result = glnext.rgba(data, 'lum')
    assert len(result) == 256
    expected = np.full((8, 8, 4), 255, 'u1')
    expected[:, :, 0] = array
    expected[:, :, 1] = array
    expected[:, :, 2] = array
    assert result == expected.tobytes()
