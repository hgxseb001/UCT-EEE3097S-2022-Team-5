import serial
import struct
ser = serial.Serial('/dev/ttyUSB0', baudrate = 9600, bytesize=serial.EIGHTBITS,parity=serial.PARITY_NONE, stopbits = serial.STOPBITS_ONE )  # open serial port
output_file = open("/home/sebastian/readSensorData.bin", "wb")
compressedSize = 0
temp = 0
for i in range(100):

	print(ser.read(1))
	
print("done")
ser.close()             # close port
output_file.close()