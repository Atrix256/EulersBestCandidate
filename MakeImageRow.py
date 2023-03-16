import numpy as np
from PIL import Image

files = [
    "out/_MBC.png",    
    "out/_EBC.png",
    ]

outFile = "BC.png"
padding = 15


textures = []

width = 0
height = 0
for file in files:
    im = Image.open(file)
    textures.append(im)
    width = width + im.size[0]
    height = max(height, im.size[1])

x = 0
width += (len(textures)-1) * padding
imOut = Image.new(textures[0].mode, (width, height), 64)
for texture in textures:
    imOut.paste(texture, (x,0))
    x = x + texture.size[0] + padding

imOut.save(outFile)
