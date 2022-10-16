import serial
import struct
ser = serial.Serial('/dev/ttyUSB0', baudrate = 9600, bytesize=serial.EIGHTBITS,parity=serial.PARITY_NONE, stopbits = serial.STOPBITS_ONE )  # open serial port
output_file = open("/home/sebastian/readSensorData.bin", "wb")
compressedSize = 0
temp = 0
for i in range(1):
	# print(ser.readline())
	# compressedSize = struct.unpack_from(">I", ser.read(4))[0]
	# ser.read(1)
	while(  temp != b'start\r\n'):
		temp = ser.readline()
		# print(temp)
	# print(ser.readline())
	temp = ser.read(4)
	# print(temp)
	compressedSize = int.from_bytes(temp, "little", signed=False)
	output_file.write(temp)

	temp = ser.read(4)
	trailingBytes = int.from_bytes(temp, "little", signed=False)
	output_file.write(temp)
	# temp = ser.read(4)
	# print(temp)
	# trailingSize = int.from_bytes(temp, "big", signed=False)
	# print(ser.readline())
	
	print(compressedSize)
	print(trailingBytes)
	# print(trailingSize)
	# print(temp)
	# print("flag0")
	temp = ser.read(compressedSize+trailingBytes)
	# print("flag0")
	output_file.write(temp)
	# print("flag1")
	# ser.read(trailingBytes) #+1 because of the added \n
	# print("flag2")
print("done")
ser.close()             # close port
output_file.close()