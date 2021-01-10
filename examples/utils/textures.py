import os
from PIL import Image

noise = Image.frombuffer('RGBA', (32, 32), os.urandom(32 * 32 * 4))
