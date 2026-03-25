import RPi.GPIO as GPIO
import serial
import struct
import time
import datetime
import argparse
import sys
import socket
import math	

GPIO.cleanup()
# настроить пины

radio_M0 = 23
radio_M1 = 24
GPIO.setmode(GPIO.BCM)
GPIO.setup(radio_M0, GPIO.OUT)
GPIO.setup(radio_M1, GPIO.OUT)

# настроить порт UART
port = "/dev/ttyRF1"    #"COM1"
baudrate = 9600
timeout = 0.1
ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
baudrate = 115200

# настроить режим работы малинки (0 - писать, 1 - писать/читать, 2 - читать, 3 - настройки)
def operating_mode (mode):
	if mode == 0:
		GPIO.output(radio_M0, GPIO.LOW)
		GPIO.output(radio_M1, GPIO.LOW)
	elif mode == 1:
		GPIO.output(radio_M0, GPIO.HIGH)
		GPIO.output(radio_M1, GPIO.LOW)
	elif mode == 2:
		GPIO.output(radio_M0, GPIO.LOW)
		GPIO.output(radio_M1, GPIO.HIGH)
	elif mode == 3:
		GPIO.output(radio_M0, GPIO.HIGH) 
		GPIO.output(radio_M1, GPIO.HIGH)
	else:
		print("Error. Number is not defined")

# отправить комманду для нстройки малинки
def command (adress, length, data):
	ser.write(struct.pack("<4B", 0xC0, adress, length, data))

# расчёт контрольной суммы
def crc16(data : bytearray, offset=0, length=-1):
	if length < 0:
		length = len(data)
	
	if data is None or offset < 0 or offset > len(data)- 1 and offset+length > len(data):
		return 0

	crc = 0xFFFF
	for i in range(0, length):
		crc ^= data[offset + i] << 8
		for j in range(0, 8):
			if (crc & 0x8000) > 0:
				crc = (crc << 1) ^ 0x1021
			else:
				crc = crc << 1
		crc = crc & 0xFFFF
	return crc & 0xFFFF

#Настройка записи в сsv и bin
def generate_logfile_name():
	now = datetime.datetime.utcnow().replace(microsecond=0)
	isostring = now.isoformat()  # string 2021-04-27T23:17:31
	isostring = isostring.replace("-", "")  # string 20210427T23:17:31
	isostring = isostring.replace(":", "")  # string 20210427T231731, òî ÷òî íàäî
	return "./log/knpn-s_binary" + isostring + ".bin"

def generate_csv_name(text):
	now = datetime.datetime.utcnow().replace(microsecond=0)
	isostring = now.isoformat()  # string 2021-04-27T23:17:31
	isostring = isostring.replace("-", "")  # string 20210427T23:17:31
	isostring = isostring.replace(":", "")  # string 20210427T231731, òî ÷òî íàäî
	return text + isostring + ".csv"

filename_f = generate_logfile_name()
filename_f_ = filename_f + ".super"
filename_MA = generate_csv_name("./log/MA_knpn-s")
filename_DA_f1 = generate_csv_name("./log/DA1_knpn-s")
filename_DA_f2 = generate_csv_name("./log/DA2_knpn-s")
filename_DA_f3 = generate_csv_name("./log/DA3_knpn-s")
f = open(filename_f, 'wb')
f.flush()
f_ = open(filename_f_, 'wb')
f_.flush()
MA = open(filename_MA, 'w')
MA.write('"Number";"Team ID";"Time_ms";"Gyro x";"Gyro y";"Gyro z";"Accel x";"Accel y";"Accel z";"Mag x";"Mag y";"Mag z";"Bme_press";"Bme_temp";"Bme_humidity";"Bme_height";"Lat";"Lon";"Height";"Fix";"Photo";"Status";"crc"\n')
MA.flush()
DA_f1 = open(filename_DA_f1, 'w')
DA_f1.write('"Number";"Time_ms";"Gyro x";"Gyro y";"Gyro z";"Accel x";"Accel y";"Accel z";"Mag x";"Mag y";"Mag z";"Status";"crc"\n')
DA_f1.flush()
DA_f2 = open(filename_DA_f2, 'w')
DA_f2.write('"Number";"Time_ms";"GPS_lat";"GPS_lon";"GPS_height";"GPS_fix";"Photo";"crc"\n')
DA_f2.flush()
DA_f3 = open(filename_DA_f3, 'w')
DA_f3.write('"Number";"Time_ms";"Bme_press_1";"Bme_press_2";"Bme_temp";"Bme_humidity";"Bme_height";"Pito_speed";"crc"\n')
DA_f3.flush()

# настройка радио
operating_mode(3)
addr_datarate = 0x02
data_rate = 0xE5
addr_channel = 0x04
channel = 0x2B
time.sleep(1)
command(addr_datarate, 0x01, data_rate)
time.sleep(5)
command(addr_channel, 0x01, channel)
time.sleep(1)
read_register = ser.read(120)
print(read_register)

ser.write([0xC1, 0x00, 0x09])
read_register = ser.read(13)

try:
	print(struct.unpack("<3BH7B", read_register[:12]))
except Exception as e:
	print(e)

operating_mode(0)
time.sleep(1)
ser.close()
ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
time.sleep(1)
print("F")
host = '0.0.0.0'
port = 22000
addr_udp = None
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_socket.setblocking(False)
udp_socket.settimeout(0.001)
print("F")
udp_socket.bind((host, port))
print("F")


#print("F")
# читаем
buf = bytes()
flug_MA = 0xAA
flug_DA_0 = 0xBB
flug_DA_1 = 0xCC
flug_DA_2 = 0xDD
while True:  
	portion = ser.read(1000)
	if portion:
		f_.write(portion)
		f_.flush()

	buf += portion

	#if len(portion) == 0:
	#	print("NO DATA")
	#else:
	#    print (portion)
	try:
		conn, addr = udp_socket.recvfrom(1)
		if len(conn) > 0:
			print("Connected to ", addr)
			addr_udp = addr
	except TimeoutError:
		pass


	try:
		while len(buf) > 0:
			flug_cond = struct.unpack("B", buf[:1])
			if flug_cond[0] == flug_MA:
			#    print("buf: ", buf)
				if len(buf) >= 55:
					
					crc_cond = crc16(buf[:53])
		
					crc = struct.unpack("H", buf[53:55])
					if crc_cond == crc[0]:
						print("==== Пакет МА ====")
						unpack_data = struct.unpack("<B2HI9hI3H3fBHBH", buf[:55])
						if addr_udp is not None:
							udp_socket.sendto(buf[:50], addr_udp)

						print ("flug", unpack_data[0])
						print ("Number", unpack_data[1])
						print ("Team ID", unpack_data[2])
						print ("Time_ms", unpack_data[3])

						print ("Gyroscope x", unpack_data[4]*70/1000)
						print ("Gyroscope y", unpack_data[5]*70/1000)
						print ("Gyroscope z", unpack_data[6]*70/1000)
						print ("Accelerometer x", unpack_data[7]*488/1000/1000)
						print ("Accelerometer y", unpack_data[8]*488/1000/1000)
						print ("Accelerometer z", unpack_data[9]*488/1000/1000)
						print ("Magnetometer x", unpack_data[10]/1711)
						print ("Magnetometer y", unpack_data[11]/1711)
						print ("Magnetometer z", unpack_data[12]/1711)
					#   print ("Gyroscope x", unpack_data[4])
					#   print ("Gyroscope y", unpack_data[5])
					#   print ("Gyroscope z", unpack_data[6])
					#   print ("Accelerometer x", unpack_data[7])
					#   print ("Accelerometer y", unpack_data[8])
					#   print ("Accelerometer z", unpack_data[9])
					#   print ("Magnetometer x", unpack_data[10])
					#   print ("Magnetometer y", unpack_data[11])
					#   print ("Magnetometer z", unpack_data[12])

						print ("Bme_press", unpack_data[13])
						print ("Bme_temp", unpack_data[14])
						print ("Bme_hum", unpack_data[15])
						print ("Bme_height", unpack_data[16])

						print ("GPS_lat", unpack_data[17])
						print ("GPS_lon", unpack_data[18])
						print ("GPS_height", unpack_data[19])
						print ("GPS_fix", unpack_data[20])

						print ("Photo", unpack_data[21])
						print ("Status", unpack_data[22])
						print ("crc", unpack_data[23])
						print ("\n")

						f.write(buf[:55])
						f.flush()

						for i in range(1,23):
							MA.write(str(unpack_data[i]))
							MA.write(";")
						MA.write('\n')
						MA.flush()

					   # print ("crc_ground", crc16(buf[:48]))
						buf = buf[50:]
					else:
						buf = buf[1:]
						break

				else:
					buf = buf[1:]
					break
			elif flug_cond[0] == flug_DA_0:
				if len(buf) >= 28:
					crc_cond = crc16(buf[:26])
		
					crc = struct.unpack("H", buf[26:28])

					if crc_cond == crc[0]:
						print("==== Пакет тип ДА 1 ====")
						unpack_data = struct.unpack("<BHI9hBH", buf[:28])
						if addr_udp is not None:
							udp_socket.sendto(buf[:28], addr_udp)
						print ("flug", unpack_data[0])
						print ("Number", unpack_data[1])
						print ("Time_ms", unpack_data[2])	

						#print ("Gyroscope x", unpack_data[3]*70/1000)
						#print ("Gyroscope y", unpack_data[4]*70/1000)
						#print ("Gyroscope z", unpack_data[5]*70/1000)
						#print ("Accelerometer x", unpack_data[6]*488/1000/1000)
						#print ("Accelerometer y", unpack_data[7]*488/1000/1000)
						#print ("Accelerometer z", unpack_data[8]*488/1000/1000)
						#print ("Magnetometer x", unpack_data[9]/1711)
						#print ("Magnetometer y", unpack_data[10]/1711)
						#print ("Magnetometer z", unpack_data[11]/1711)
						print ("Gyroscope x", unpack_data[3])
						print ("Gyroscope y", unpack_data[4])
						print ("Gyroscope z", unpack_data[5])
						print ("Accelerometer x", unpack_data[6])
						print ("Accelerometer y", unpack_data[7])
						print ("Accelerometer z", unpack_data[8])
						print ("Magnetometer x", unpack_data[9])
						print ("Magnetometer y", unpack_data[10])
						print ("Magnetometer z", unpack_data[11])

						print ("Status", unpack_data[12])
						print ("crc", unpack_data[13])
						print ("\n")

						f.write(buf[:28])
						f.flush()

						for i in range(1,13):
							DA_f1.write(str(unpack_data[i]))
							DA_f1.write(";")
						DA_f1.write('\n')
						DA_f1.flush()


					  #  print ("crc_ground", crc16(buf[:51]))
						buf = buf[28:]  
					else:
						buf = buf[1:]
						break
				else:
					buf = buf[1:]
					break 
			elif flug_cond[0] == flug_DA_1:
				if len(buf) >= 24:
					crc_cond = crc16(buf[:22])
					crc = struct.unpack("H", buf[22:24])
					if crc_cond == crc[0]:
						print("==== Пакет тип ДА 2 ====")
						unpack_data = struct.unpack("<BHI3fB2H", buf[:24])
						if addr_udp is not None:
							udp_socket.sendto(buf[:24], addr_udp)
						print ("flug", unpack_data[0])
						print ("Number", unpack_data[1])
						print ("Time_ms", unpack_data[2])

						print ("GPS_lat", unpack_data[3])
						print ("GPS_lon", unpack_data[4])
						print ("GPS_height", unpack_data[5])
						print ("GPS_fix", unpack_data[6])
						
						print ("Photo", unpack_data[7])
						print ("crc", unpack_data[8])
						print ("\n")

						f.write(buf[:24])
						f.flush()
						for i in range(1,8):
							DA_f2.write(str(unpack_data[i]))
							DA_f2.write(";")
						DA_f2.write('\n')
						DA_f2.flush()
					  #  print ("crc_ground", crc16(buf[:51]))
						buf = buf[24:]
					else:
						buf = buf[1:]
						break
				else:
					buf = buf[1:]
					break 
			elif flug_cond[0] == flug_DA_2:
				if len(buf) >= 24:
					crc_cond = crc16(buf[:22])
					crc = struct.unpack("H", buf[22:24])
					if crc_cond == crc[0]:
						print("==== Пакет тип ДА 3 ====")
						unpack_data = struct.unpack("<BHI2f3HBH", buf[:24])
						if addr_udp is not None:
							udp_socket.sendto(buf[:24], addr_udp)
						print ("flug", unpack_data[0])
						print ("Number", unpack_data[1])
						print ("Time_ms", unpack_data[2])	
						
						print ("Bme_press_1", unpack_data[3])
						print ("Bme_press_2", unpack_data[4])
						print ("Bme_temp", unpack_data[5])
						print ("Bme_humidity", unpack_data[6])
						print ("Bme_height", unpack_data[7])

						print ("Pito_speed", unpack_data[8])
						print ("crc", unpack_data[9])
						print ("\n")

						f.write(buf[:24])
						f.flush()
						for i in range(1,9):
							DA_f3.write(str(unpack_data[i]))
							DA_f3.write(";")
						DA_f3.write('\n')
						DA_f3.flush()
					  #  print ("crc_ground", crc16(buf[:51]))
						buf = buf[28:]  
					else:
						buf = buf[1:]
						break
				else:
					buf = buf[1:]
					break 
			else:
				buf = buf[1:]
				continue
	except Exception as e:
		print("ERROR: %s", e)
		print ('\n')
		buf = bytes()