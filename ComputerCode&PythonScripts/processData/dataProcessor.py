import csv
#import os
#import decimal
from numpy import array
import numpy as np
from numpy.fft import fft, ifft

import matplotlib.pyplot as plt

import filecmp

data_file = 2 #1 for data 1, 2 for data 2, 3 for data 3
sensor_reading = 0 # choose number from 0 to 5

noise_amplitude = 0.000000001



def compareFiles(file1, file2):
    return filecmp.cmp(file1,file2)
    

avg_sample_time = 0.012507443106369

num_samples = 2**11

#this is used for reading data from the compression algorithm
num_bytes = num_samples * 4 * 6 #each sample consists of 6 numbers, each made of 4 bytes

n = np.arange(num_samples)

freq = n / avg_sample_time

t = n * avg_sample_time



#for f in os.listdir("/home/sebastian/EEE3097_Compression/processData"):
    #print(f)

file = open("/home/sebastian/EEE3097S/EEE3097_Compression/processData/data{}.csv".format(data_file))

reader = csv.reader(file, delimiter=',')
    
header = []
header = next(reader)


#print(header)

rows = []
data = []

for row in reader:
    rows.append(row)
    data.append([])

file.close()

#data = []
noise = np.random.normal(0,noise_amplitude, len(rows))

for i in range(len(rows)):
    for j in range(6, 12,1):
        if rows[i][j] == '':
            data[i].append(0.0)
        else:
            #data[i].append(decimal.Decimal(float(rows[i][j])).quantize(decimal.Decimal('1.00')))
            data[i].append(float(rows[i][j])+ noise[i])

#print(data)

output_file = open("/home/sebastian/EEE3097S/EEE3097_Compression/processData/Data{}.bin".format(data_file), "wb")

maxSize = num_samples

if len(data) < maxSize:
    maxSize = len(data)

del data[maxSize:]
#print(data)

if compareFiles("/home/sebastian/EEE3097S/EEE3097_Compression/processData/Data{}.bin".format(data_file),"/home/sebastian/EEE3097S/EEE3097_Compression/destinationFile.bin" ):
    print("files are equal")
else:
    print("files are not equal")

tempArray = array(data, "float32")
tempArray.tofile(output_file)

output_file.close()

acc_x = []



for i in range(len(data)):

    acc_x.append(data[i][sensor_reading])

acc_X = fft(acc_x)

plt.figure(figsize=(18, 12))
plt.subplot(121)
half_fft = num_samples / (2*avg_sample_time)
plt.xlim(0, half_fft / 32)
plt.ylim(0, 40)
plt.title("Frequency Domain Representation of X-Axis Accelerometer")
plt.xlabel("Frequency [mHz]")
plt.ylabel("Magnitude")
plt.plot(freq, np.abs(acc_X))
plt.subplot(122)
plt.title("Time Domain Representation of X-Axis Accelerometer")
plt.xlabel("Time [s]")
plt.ylabel("Magnitude")
plt.plot(t, acc_x)
plt.subplot(122)
"""plt.title("Histogram Representation of X-Axis Accelerometer")
plt.xlabel("Value")
plt.ylabel("Occurance")
plt.hist(np.abs(acc_x),100)"""
plt.show()