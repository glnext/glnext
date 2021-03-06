import glnext

instance = glnext.instance()

buffer_1 = instance.buffer('storage_buffer', 64, readable=True)
buffer_2 = instance.buffer('storage_buffer', 64, readable=True)
buffer_3 = instance.buffer('storage_buffer', 64, readable=True)
image_1 = instance.image((4, 4), mode='output')
image_2 = instance.image((4, 4), mode='output')

group = instance.group(buffer=1024)

with group:
    buffer_1.write(b'a' * 64)
    buffer_2.write(b'b' * 64)
    buffer_3.write(b'c' * 64)
    image_1.write(b'x' * 64)
    image_2.write(b'y' * 64)
    image_1.read()
    image_2.read()
    buffer_1.read()

print(bytes(group.output[0]))
print(bytes(group.output[1]))
print(bytes(group.output[2]))
