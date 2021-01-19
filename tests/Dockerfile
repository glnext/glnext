FROM python:3.9.1
RUN apt-get update && apt-get install -y libvulkan-dev vulkan-utils vulkan-validationlayers
RUN python3 -m pip install Pillow pytest
ENV VK_ICD_FILENAMES=/swiftshader/vk_swiftshader_icd.json PYTHONUNBUFFERED=1
ENV TEST_VULKAN_LAYERS="VK_LAYER_LUNARG_core_validation VK_LAYER_LUNARG_parameter_validation VK_LAYER_LUNARG_standard_validation"
COPY swiftshader /swiftshader

WORKDIR /build
COPY glnext.cpp setup.py MANIFEST.in README.md LICENSE /build/
RUN python3 setup.py sdist
RUN python3 -m pip wheel dist/glnext-*.tar.gz
RUN python3 -m pip install glnext-*.whl

WORKDIR /tests
COPY tests /tests
CMD python3 -m pytest
