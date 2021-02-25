import os
import platform
from setuptools import Extension, setup

PLATFORMS = {'windows', 'linux', 'darwin'}

target = platform.system().lower()

for known in PLATFORMS:
    if target.startswith(known):
        target = known
        break

define_macros = [
    ('BUILD_' + target.upper(), None),
    ('PY_SSIZE_T_CLEAN', None),
    ('VK_NO_PROTOTYPES', None),
]
include_dirs = ['./include']
extra_compile_args = []
extra_link_args = []
libraries = []

# if target == 'windows':
#     extra_compile_args.append('/Z7')
#     extra_link_args.append('/DEBUG:FULL')

if target == 'linux':
    extra_compile_args.append('-fpermissive')
    libraries.append('dl')

glnext = Extension(
    name='glnext',
    sources=['glnext/glnext.cpp'],
    depends=[
        'glnext/batch.cpp',
        'glnext/binding.cpp',
        'glnext/buffer.cpp',
        'glnext/compute_pipeline.cpp',
        'glnext/debug.cpp',
        'glnext/extension.cpp',
        'glnext/framebuffer.cpp',
        'glnext/glnext.hpp',
        'glnext/image.cpp',
        'glnext/info.cpp',
        'glnext/instance.cpp',
        'glnext/loader.cpp',
        'glnext/render_pipeline.cpp',
        'glnext/staging_buffer.cpp',
        'glnext/surface.cpp',
        'glnext/tools.cpp',
        'glnext/utils.cpp',
    ],
    define_macros=define_macros,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
    include_dirs=include_dirs,
    libraries=libraries,
)

with open('README.md') as readme:
    long_description = readme.read()

setup(
    name='glnext',
    version='0.7.0',
    ext_modules=[glnext],
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/cprogrammer1994/glnext',
    author='Szabolcs Dombi',
    author_email='cprogrammer1994@gmail.com',
    license='MIT',
)
