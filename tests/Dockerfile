FROM python:3.9.2
RUN wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add - &&\
    wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.170-bionic.list https://packages.lunarg.com/vulkan/1.2.170/lunarg-vulkan-1.2.170-bionic.list &&\
    apt-get update && apt-get install -y vulkan-headers vulkan-tools vulkan-validationlayers libvulkan-dev shaderc
RUN python3 -m pip install pytest==6.2.2 pillow==8.1.0 numpy==1.20.1 sphinx==3.5.1 sphinx_rtd_theme==0.5.1 glnext_compiler==1.0.1
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
