import os
import platform
from setuptools import Extension, setup

PLATFORMS = {'windows', 'linux', 'darwin'}

target = platform.system().lower()

for known in PLATFORMS:
    if target.startswith(known):
        target = known
        break

define_macros = [('BUILD_' + target.upper(), None)]
extra_compile_args = []
extra_link_args = []
include_dirs = []
library_dirs = []
libraries = []

if target == 'windows':
    # extra_compile_args.append('/Z7')
    # extra_link_args.append('/DEBUG:FULL')
    include_dirs.append(os.path.join(os.getenv('VULKAN_SDK'), 'Include'))
    library_dirs.append(os.path.join(os.getenv('VULKAN_SDK'), 'Lib'))
    libraries.append('vulkan-1')

if target == 'linux':
    extra_compile_args.append('-fpermissive')
    libraries.append('vulkan')

glnext = Extension(
    name='glnext',
    sources=['./glnext.cpp'],
    define_macros=define_macros,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
    include_dirs=include_dirs,
    library_dirs=library_dirs,
    libraries=libraries,
)

setup(
    name='glnext',
    version='0.1.0',
    ext_modules=[glnext],
)
