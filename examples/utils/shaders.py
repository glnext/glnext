import shaderc


def compile_shader(filename):
    return shaderc.compile(filename)
