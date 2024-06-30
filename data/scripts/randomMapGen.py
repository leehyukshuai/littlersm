from PIL import Image
import random
import math

img = Image.new('RGB', (600, 1), 'white')

pixels = img.load()

pi = math.pi

random.seed(10086)

for i in range(img.width):
    for j in range(img.height):
        e1 = random.random()
        e2 = random.random() * math.pi * 2
        pixels[i,j] = (int((e1 * math.sin(e2) + 1) / 2 * 255), int((e1 * math.cos(e2) + 1) / 2 * 255), int(e1 ** 2 * 255))

img.save('../images/random_map.png')