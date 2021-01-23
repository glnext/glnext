FROM python:3.9.1
RUN wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add - &&\
    echo "deb https://packages.lunarg.com/vulkan/1.2.162 bionic main" >> /etc/apt/sources.list &&\
    apt-get update && apt-get install -y vulkan-headers vulkan-tools vulkan-validationlayers libvulkan-dev
RUN python3 -m pip install Pillow pytest
ENV VK_ICD_FILENAMES=/swiftshader/vk_swiftshader_icd.json PYTHONUNBUFFERED=1
COPY swiftshader /swiftshader

WORKDIR /build
COPY glnext.cpp setup.py MANIFEST.in README.md LICENSE /build/
RUN python3 setup.py sdist
RUN python3 -m pip wheel dist/glnext-*.tar.gz
RUN python3 -m pip install glnext-*.whl

WORKDIR /tests
COPY tests /tests
CMD python3 -m pytest
