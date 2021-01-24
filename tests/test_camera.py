import glnext
import numpy as np


def test_simple_camera():
    camera = glnext.camera((4.0, 3.0, 2.0), (0.0, 0.0, 0.0))
    assert type(camera) is bytes
    assert len(camera) == 64

    array = np.ndarray((4, 4), 'f4', camera)
    expected = [
        [-0.7819, -0.3872, -0.7429, -0.7427],
        [1.0426, -0.2904, -0.5572, -0.5571],
        [0.0000, 1.2100, -0.3715, -0.3714],
        [0.0000, 0.0000, 5.1862, 5.3852],
    ]
    np.testing.assert_array_almost_equal(array, expected, decimal=2)
