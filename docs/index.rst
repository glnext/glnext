glnext
======

.. code::

    pip install glnext

- `Documentation <https://glnext.readthedocs.io/>`_
- `glnext on Github <https://github.com/glnext/glnext/>`_
- `glnext on PyPI <https://pypi.org/project/glnext/>`_

Objects
=======

.. py:class:: Instance

| Represents a single Vulkan instance.

.. py:class:: Surface

| Represents a Vulkan compatible window surface and an image source.
| The sole purpose of this object is to allow swapping the image source with :py:attr:`Surface.image`.
| Presentation won't take place until :py:meth:`Instance.present` is called.

.. py:class:: Task

| Represents a collection of Vulkan objects and operations.

.. py:class:: Framebuffer

| Represents an entire Render Pass and Framebuffer.
| Graphics Piplines are created from this object.
| Compute Pipelines created from this object can be used to post process the framebuffer output.
| They run before the final layout transition takes place.

.. py:class:: RenderPipeline

| Represents a single Graphics Pipeline and a single draw call per Framebuffer layer.
| RenderPipelines are automatically executed in their order of creation when :py:meth:`Task.run()` is called.

.. py:class:: ComputePipeline

| Represents a single Compute Pipeline and a single dispatch call.
| ComputePipeline are automatically executed in their order of creation when :py:meth:`Task.run()` is called.

.. py:class:: Group

| Represents a context helper for grouping read and write operations and task runs.
| The command buffer is in recording state during the group is active.
| On ``__exit__`` the command buffer is executed and the output memory views are filled with data.

.. py:class:: Buffer

| Represents a device local buffer objects.

.. py:class:: Image

| Represents a device local image objects.

.. py:class:: Memory

| Represents a Vulkan memory allocation.
| This class is for advanced use only.

Diagrams
========

Object Hierarchy
----------------

.. image:: images/diagram_5.png

.. image:: images/diagram_4.png

Task Execution
--------------

.. image:: images/diagram_6.png

Framebuffer execution
---------------------

.. image:: images/diagram_1.png

Framebuffer Memory
------------------

.. image:: images/diagram_2.png

.. image:: images/diagram_3.png

Documentation
=============

Instance objects
----------------

.. py:method:: glnext.instance(physical_device:int=0, application_name:str=None, application_version:int=0, engine_name:str=None, engine_version:int=0, backend:str=None, surface:bool=False, layers:list=None, cache:bytes=None, debug:bool=False) -> Instance

.. py:method:: Instance.surface(window: tuple, image: Image) -> Surface

.. py:method:: Instance.buffer(type: str, size: int, readable:bool=False, writable:bool=True, memory:Memory=None) -> Buffer

.. py:method:: Instance.image(size:tuple, format:str='4p', levels:int=1, layers:int=1, mode:str='output', memory:Memory=None) -> Image

.. py:method:: Instance.task() -> Task

.. py:method:: Instance.group() -> Group

| Prevent submitting small command buffers for each read and write call.
| Wrap multiple operations with this context helper.
| The write operations copy immediately into the staging buffer.
| The read and write operations complete only on ``__exit__``.
| All the operations within a group respect ordering.

More on this at `How glnext groups work? <#>`_

.. py:method:: Instance.present()

| For every surface the source image content is blitted to the acquired swapchain image.
| The invalid swapchains silently destroy for the closed windows.
| The user is not responsible for cleaning up after window recreation, the resources are automatically freed.
| This call may be blocking until vsync.

More on this at `How glnext present works? <#>`_

.. py:method:: Instance.cache() -> bytes

| Returns the pipeline cache content.
| The pipeline cache is always empty if the instance was created with ``cache=None``.
| Initialize the :py:class:`Instance` with ``cache=data`` to enable the pipeline cache.

Reasoning behind the cache parameter and the cache function:

There are two best practices to follow:

1. Have a single pipeline cache.

    | When having a pipeline cache the user is expected to provide its content when creating the instance.
    | This can be done with the following code:

    .. code-block::

        my_pipeline_cache_data = b''
        if os.path.exists('my_pipeline_cache.cache'):
            my_pipeline_cache_data = open('my_pipeline_cache.cache', 'rb').read()

        instance = glnext.instance(cache=my_pipeline_cache_data)

        # ... the program goes here

        open('my_pipeline_cache.cache', 'wb').write(instance.cache())

    | In this example the cache parameter is always ``bytes`` even on the first run.
    | There are better ways to load and store binary data. This example is just a short working example.

2. Have no caching at all.

    | To disable the pipeline cache entirely set the cache to ``None``.
    | An :py:class:`Instance` created with ``cache=None`` will not generate cache on pipeline creation
      and the :py:meth:`Instance.cache` will fail with an error.

Surface objects
---------------

.. py:attribute:: Surface.image
    :type: Image

| A surface source image can be replaced by setting the :py:attr:`Surface.image` attribute to a different image.
| The new image must have the same size and format as the original image.
| Setting the image is a cheap operation.

Task objects
------------

.. py:method:: Task.framebuffer(size:tuple, format:str='4p', samples:int=4, levels:int=1, layers:int=1, depth:bool=True, compute:bool=False, mode:str='output', memory:Memory=None) -> Framebuffer

.. py:method:: Task.compute(compute_shader:bytes, compute_count:tuple, bindings:list, memory:Memory=None) -> ComputePipeline

.. py:method:: Task.run()

| Executes all :py:class:`RenderPipeline` and :py:class:`ComputePipeline` objects derived from this objects.
| This call may be blocking until all the operations finish.

Framebuffer objects
-------------------

.. py:method:: Framebuffer.render(vertex_shader, fragment_shader, task_shader, mesh_shader, vertex_format, instance_format, vertex_count, instance_count, index_count, indirect_count, max_draw_count, vertex_buffer, instance_buffer, index_buffer, indirect_buffer, count_buffer, vertex_buffer_offset, instance_buffer_offset, index_buffer_offset, indirect_buffer_offset, count_buffer_offset, topology, restart_index, short_index, depth_test, depth_write, bindings, memory) -> RenderPipeline

.. py:method:: Framebuffer.compute(compute_shader:bytes, compute_count:tuple, bindings:list, memory:Memory=None) -> ComputePipeline

.. toctree::
   :maxdepth: 2
   :caption: Contents:

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
