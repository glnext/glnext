import os

import glnext
import numpy as np
from glnext_compiler import glsl
from PIL import Image


def pebble(segs):
    xn = segs
    yn = segs * 4
    X = np.repeat(np.linspace(0.0, np.pi / 2.0, xn, endpoint=False), yn)
    Y = np.tile(np.linspace(0.0, np.pi * 2.0, yn, endpoint=False), xn)
    V = np.array([np.cos(X) * np.cos(Y), np.cos(X) * np.sin(Y), np.sin(X)]).T
    V = np.concatenate([V, [[0.0, 0.0, 1.0]]])
    X = np.repeat(np.arange(xn), yn) * yn
    X1 = X[:-yn]
    X2 = X[yn:]
    Y1 = np.tile(np.arange(yn), xn - 1)
    Y2 = np.roll(Y1, 1)
    I1 = np.array([X1 + Y1, X2 + Y1, X1 + Y2]).T
    I2 = np.array([X1 + Y2, X2 + Y2, X2 + Y1]).T
    C1 = Y1[:yn] + (xn - 1) * yn
    C2 = np.roll(Y1[:yn] + (xn - 1) * yn, 1)
    C3 = np.full(yn, xn * yn)
    I = np.concatenate([I1, I2, np.array([C1, C2, C3]).T])
    return V.astype('f4').tobytes(), I.astype('i4').tobytes()


def fill(count):
    X1 = np.random.uniform(0.0, np.pi, count)
    X2 = np.random.normal(0.0, 1.0, count)
    X = np.array([np.cos(X1) * X2, np.sin(X1) * X2]).T
    R = np.random.uniform(0.2, 0.4, count)
    A = np.repeat(np.arange(count), count - 1).reshape(count, count - 1)
    B = ((np.arange(count * count) + np.repeat(np.arange(count), count) + 1) % count).reshape(count, count)[:,:-1]
    for _ in range(1000):
        C = X[A] - X[B]
        D = np.sqrt(np.sum(np.square(C), axis=2))
        D2 = np.array([D, D]).transpose(1, 2, 0)
        R2 = np.array([R[A] + R[B], R[A] + R[B]]).transpose(1, 2, 0) * 0.85
        X += np.sum(C / D2 * R2 * (D2 < R2) * 0.01, axis=1)
    Z = np.zeros(count)
    SZ = np.random.normal(1.0, 0.1, count) * 0.3
    return np.array([X[:,0], X[:,1], Z, R, R, R * SZ]).T.astype('f4').tobytes()


instance = glnext.instance(surface=True)

framebuffer = instance.framebuffer((512, 512), '4p 4p 4p 4p 4p')

vertex_data, index_data = pebble(10)
instance_data = fill(100)

pipeline = framebuffer.render(
    vertex_shader=glsl('''
        #version 450
        #pragma shader_stage(vertex)

        layout (binding = 0) uniform Buffer {
            mat4 mvp;
        };

        layout (location = 0) in vec3 in_vert;
        layout (location = 1) in vec3 in_pos;
        layout (location = 2) in vec3 in_scale;

        layout (location = 0) out vec4 out_vert;
        layout (location = 1) out vec3 out_norm;

        void main() {
            out_vert = vec4(in_pos + in_vert * in_scale, 1.0);
            gl_Position = mvp * out_vert;
            out_norm = normalize(in_vert);
        }
    '''),
    fragment_shader=glsl('''
        #version 450
        #pragma shader_stage(fragment)

        layout (binding = 0) uniform Buffer {
            mat4 mvp;
        };

        layout (location = 0) in vec4 in_vert;
        layout (location = 1) in vec3 in_norm;

        layout (location = 0) out vec3 out_color;
        layout (location = 1) out vec3 out_diffuse;
        layout (location = 2) out vec3 out_normal;
        layout (location = 3) out float out_bump;
        layout (location = 4) out float out_alpha;

        void main() {
            float lum = dot(normalize(vec3(0.2, 0.3, 1.0)), normalize(in_norm));
            lum = lum * 0.7 + 0.3;
            out_diffuse = vec3(1.0, 1.0, 1.0);
            out_normal = normalize(in_norm) * 0.5 + 0.5;
            out_bump = in_vert.z;
            out_alpha = 1.0;
            out_color = out_diffuse * lum;
        }
    '''),
    vertex_format='3f',
    instance_format='3f 3f',
    vertex_count=len(vertex_data) // 12,
    index_count=len(index_data) // 4,
    instance_count=len(instance_data) // 24,
    bindings=[
        {
            'binding': 0,
            'name': 'uniform_buffer',
            'type': 'uniform_buffer',
            'size': 64,
        },
    ],
)

framebuffer.update(
    clear_values=glnext.pack([
        1.0, 1.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
        0.5, 0.5, 0.5, 0.0,
        0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
    ]),
)

pipeline.update(
    uniform_buffer=glnext.camera((0.0, 0.0, 100.0), (0.0, 0.0, 0.0), (0.0, 1.0, 0.0), fov=4.0),
    vertex_buffer=vertex_data,
    index_buffer=index_data,
    instance_buffer=instance_data,
)


def image(output):
    data = framebuffer.output[output].read()
    return Image.frombuffer('RGBX', (512, 512), data, 'raw', 'RGBX', 0, -1).convert('RGB')


instance.run()
image(0).show()

image(1).save('examples/res/pebble_diffuse.png')
image(2).save('examples/res/pebble_normal.png')
image(3).getchannel('R').save('examples/res/pebble_bump.png')
image(4).getchannel('R').save('examples/res/pebble_alpha.png')
