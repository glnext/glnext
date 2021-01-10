import os
from setuptools import Extension, setup

ext = Extension(
    name='glnext',
    sources=['./glnext.cpp'],
    include_dirs=[os.path.join(os.getenv('VULKAN_SDK'), 'Include')],
    library_dirs=[os.path.join(os.getenv('VULKAN_SDK'), 'Lib')],
    libraries=['vulkan-1'],
)

setup(
    name='glnext',
    version='0.1.0',
    ext_modules=[ext],
)
