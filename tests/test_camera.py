import glnext
import numpy as np


def test_simple_camera():
    camera = glnext.camera((4.0, 3.0, 2.0), (0.0, 0.0, 0.0), fov=60.0)
    assert type(camera) is bytes
    assert len(camera) == 64

    array = np.ndarray((4, 4), 'f4', camera)
    expected = [
        [-1.0392, -0.5146, -0.7428, -0.7427],
        [1.3856, -0.3859, -0.5571, -0.5570],
        [0.0000, 1.6081, -0.3714, -0.3713],
        [0.0000, 0.0000, 5.2856, 5.3851]
    ]
    np.testing.assert_array_almost_equal(array, expected, decimal=2)


def test_ortho_camera():
    camera = glnext.camera((4.0, 3.0, 2.0), (0.0, 0.0, 0.0), fov=0.0)
    assert type(camera) is bytes
    assert len(camera) == 64

    array = np.ndarray((4, 4), 'f4', camera)
    expected = [
        [-0.6000, -0.2971, -0.0007, 0.0000],
        [0.8000, -0.2228, -0.0005, 0.0000],
        [0.0000, 0.9284, -0.0003, 0.0000],
        [0.0000, 0.0000, 0.0052, 1.0000]
    ]
    np.testing.assert_array_almost_equal(array, expected, decimal=2)
