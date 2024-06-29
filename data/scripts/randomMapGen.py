from PIL import Image
import random


img = Image.new('RGB', (600, 1), 'white')

pixels = img.load()

for i in range(img.width):
    for j in range(img.height):
        pixels[i,j] = (random.randint(0,255),random.randint(0,255),random.randint(0,255))
        
img.save('random_map.png')