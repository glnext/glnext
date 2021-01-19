import glnext
import pytest


def test_renderer(instance):
    renderer = instance.renderer((4, 4), mode='output')
    assert len(renderer.output), 1
    assert renderer.output[0] is not None


def test_renderer_multiple_outputs(instance):
    renderer = instance.renderer((4, 4), '4b 4b 4b', mode='output')
    assert len(renderer.output), 3


def test_renderer_layered(instance):
    renderer = instance.renderer((4, 4), layers=6, mode='output')
    assert len(renderer.output), 1


def test_renderer_read(instance):
    renderer = instance.renderer((4, 4), mode='output')
    instance.render()
    assert renderer.output[0].read().hex() == '00000000' * 16


def test_renderer_clear_depth(instance):
    renderer = instance.renderer((4, 4), depth=True, mode='output')
    renderer.update(
        clear_depth=0.5,
        clear_color=glnext.pack([
            1.0, 1.0, 1.0, 1.0,
        ]),
    )
    instance.render()
    assert renderer.output[0].read().hex() == 'ffffffff' * 16


def test_renderer_clear_color_overflow(instance):
    renderer = instance.renderer((4, 4), '4b 4b', mode='output')
    with pytest.raises(ValueError):
        renderer.update(
            clear_color=glnext.pack([
                1.0, 1.0, 1.0, 1.0,
                1.0, 1.0, 1.0, 1.0,
                1.0, 1.0,
            ]),
        )


def test_renderer_clear_color_type_error(instance):
    renderer = instance.renderer((4, 4), mode='output')

    with pytest.raises(TypeError):
        renderer.update(
            clear_color=[1.0, 1.0, 1.0, 1.0],
        )

    with pytest.raises(TypeError):
        renderer.update(
            clear_color='0000000000000000',
        )

    with pytest.raises(TypeError):
        renderer.update(
            clear_color=1.0,
        )


def test_renderer_clear_depth_type_error(instance):
    renderer = instance.renderer((4, 4), mode='output')

    with pytest.raises(TypeError):
        renderer.update(
            clear_depth=100,
        )

    with pytest.raises(TypeError):
        renderer.update(
            clear_depth='',
        )


def test_renderer_clear_color_underflow(instance):
    renderer = instance.renderer((4, 4), '4b 4b', mode='output')
    with pytest.raises(ValueError):
        renderer.update(
            clear_color=glnext.pack([
                1.0, 1.0, 1.0, 1.0,
                1.0, 1.0,
            ]),
        )


def test_renderer_clear_color(instance):
    renderer = instance.renderer((4, 4), '4b 4b 4b 4b', depth=False, mode='output')
    renderer.update(clear_color=glnext.pack([
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ]))
    instance.render()
    assert renderer.output[0].read().hex() == '0000ff00' * 16
    assert renderer.output[1].read().hex() == '00ff0000' * 16
    assert renderer.output[2].read().hex() == 'ff000000' * 16
    assert renderer.output[3].read().hex() == '000000ff' * 16


def test_renderer_clear_color_integer(instance):
    renderer = instance.renderer((4, 4), '2i', depth=False, mode='output')
    renderer.update(clear_color=glnext.pack('2i 8x', [
        0x12345678, 0xaabbaabb,
    ]))
    instance.render()
    assert renderer.output[0].read().hex() == '78563412bbaabbaa' * 16
