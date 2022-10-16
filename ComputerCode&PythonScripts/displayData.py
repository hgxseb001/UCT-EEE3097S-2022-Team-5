from curses.ascii import FS
import matplotlib.pyplot as plt
import struct
import numpy as np
import os

data = []
x = []
sourceFile = open("readOut.bin", 'rb')

fSize = os.stat("readOut.bin").st_size
byte = sourceFile.read(fSize)

data = np.frombuffer(byte, dtype = np.float16)
# print(data)
for i in range(int(fSize/2)):
    x.append(i)

plt.figure(figsize=(18, 12))
plt.subplot(121)
plt.plot(x, data)
plt.show()
