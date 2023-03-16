import sys
from PIL import Image
import numpy as np

baseFileName = sys.argv[1]
fileCount = int(sys.argv[2])
outFileName = sys.argv[3]

result = None

# for each image
for i in range(fileCount):

    # load the image
    fileName = baseFileName + str(i) + ".png"
    im = np.array(Image.open(fileName), dtype=float) / 255.0
    #im = im * 2.0 - 1.0

    # get the magnitude, zero out DC and shift it so DC is in the middle
    dft = abs(np.fft.fft2(im))
    dft[0,0] = 0.0
    dft = np.fft.fftshift(dft)

    # average
    if result is None:
        result = dft
    else:
        alpha = 1.0 / float(i+1)
        result = result * (1.0 - alpha) + dft * alpha

# log and normalize
imOut = result #np.log(1+result)
themin = np.min(imOut)
themax = np.max(imOut)
if themin != themax:
    imOut = (imOut - themin) / (themax - themin)
else:
    imOut = imOut

Image.fromarray((imOut*255.0).astype(np.uint8), mode="L").save(outFileName)
