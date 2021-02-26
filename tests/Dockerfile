FROM python:3.9.1
RUN wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add - &&\
    echo "deb https://packages.lunarg.com/vulkan/1.2.162 bionic main" >> /etc/apt/sources.list &&\
    apt-get update && apt-get install -y vulkan-headers vulkan-tools vulkan-validationlayers libvulkan-dev shaderc
RUN python3 -m pip install pytest pillow numpy sphinx sphinx_rtd_theme glnext_compiler==0.3.0
ENV VK_ICD_FILENAMES=/swiftshader/vk_swiftshader_icd.json PYTHONUNBUFFERED=1
COPY swiftshader /swiftshader

WORKDIR /build
COPY docs /build/docs
COPY include /build/include
COPY setup.py MANIFEST.in README.md LICENSE /build/
COPY glnext /build/glnext
RUN python3 setup.py sdist
RUN python3 setup.py build_sphinx
RUN python3 -m pip wheel dist/glnext-*.tar.gz
RUN python3 -m pip install glnext-*.whl

WORKDIR /tests
COPY tests /tests
CMD python3 -m pytest
