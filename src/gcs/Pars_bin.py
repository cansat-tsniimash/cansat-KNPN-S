import struct
import time
import datetime
import argparse
import sys

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

f = open(sys.argv[1], 'rb')
MA = open(sys.argv[1] + "MA.csv", "w")
MA.write('"Flug";"Number";"Team ID";"Time_ms";"Gyro x";"Gyro y";"Gyro z";"Accel x";"Accel y";"Accel z";"Mag x";"Mag y";"Mag z";"Bme_press";"Bme_temp";"Bme_humidity";"Bme_height";"Lat";"Lon";"Height";"Fix";"Photo";"Status";"crc"\n')
DA_f1 = open(sys.argv[1] + "DA_1.csv", "w")
DA_f1.write('"Flug";"Number";"Time_ms";"Gyro x";"Gyro y";"Gyro z";"Accel x";"Accel y";"Accel z";"Mag x";"Mag y";"Mag z";"Status";"crc"\n')
DA_f2 = open(sys.argv[1] + "DA_2.csv", 'w')
DA_f2.write('"Flug";"Number";"Time_ms";"GPS_lat";"GPS_lon";"GPS_height";"GPS_fix";"Photo";"crc"\n')
DA_f3 = open(sys.argv[1] + "DA_3.csv", 'w')
DA_f3.write('"Flug";"Number";"Time_ms";"Bme_press_1";"Bme_press_2";"Bme_temp_1";"Bme_temp_2";"Bme_humidity_1";Bme_humidity_2;"Bme_height";"Pito_speed";"crc"\n')


addr_udp = None


flug_MA = 0xAA
flug_DA_0 = 0xBB
flug_DA_1 = 0xCC
flug_DA_2 = 0xDD

buf = bytes()
while True: 
	portion = f.read(10000)
	buf += portion
	if not buf:
		break

	flug_cond = struct.unpack("B", buf[:1])
	if flug_cond[0] == flug_MA:
		MA_SIZE = 55
		if len(buf) < MA_SIZE:
			print("==== Пакет МА ==== ошибка длины")
			buf = buf[1:]
			continue

		crc_cond = crc16(buf[:MA_SIZE - 2])
		crc = struct.unpack("H", buf[MA_SIZE - 2 : MA_SIZE])
		if crc_cond != crc[0]:
			print("==== Пакет МА ==== ошибка CRC")
			#buf = buf[1:]
			#continue

		print("==== Пакет МА ====")
		unpack_data = struct.unpack("<B2HI9hIh2H3fBHBH", buf[:MA_SIZE])

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

		MA.write(";".join([str(x).replace(".", ",") for x in unpack_data]) + "\n")

		MA.write('\n')
		MA.flush()
		buf = buf[MA_SIZE:]

	elif flug_cond[0] == flug_DA_0:
		DA0_SIZE = 28
		if len(buf) < DA0_SIZE:
			print("==== Пакет тип ДА 1 ==== Ошибка длины")
			buf = buf[1:]
			continue

		crc_cond = crc16(buf[:DA0_SIZE - 2])
		crc = struct.unpack("H", buf[DA0_SIZE - 2 : DA0_SIZE])
		if crc_cond == crc[0]:
			print("==== Пакет тип ДА 1 ==== Ошибка CRC")
			#buf = buf[1:]
			#continue

		print("==== Пакет тип ДА 1 ====")
		unpack_data = list(struct.unpack("<BHI9hBH", buf[:DA0_SIZE]))
		if addr_udp is not None:
			udp_socket.sendto(buf[:DA0_SIZE], addr_udp)
		print ("flug", unpack_data[0])
		print ("Number", unpack_data[1])
		print ("Time_ms", unpack_data[2])	

		for i in range(3):
			unpack_data[3 + i] = unpack_data[3 + i]*70/1000
		print ("Gyroscope x", unpack_data[3])
		print ("Gyroscope y", unpack_data[4])
		print ("Gyroscope z", unpack_data[5])
		for i in range(3):
			unpack_data[6 + i] = unpack_data[6 + i]*488/1000/1000
		print ("Accelerometer x", unpack_data[6])
		print ("Accelerometer y", unpack_data[7])
		print ("Accelerometer z", unpack_data[8])
		for i in range(3):
			unpack_data[9 + i] = unpack_data[9 + i]/1711
		print ("Magnetometer x", unpack_data[9])
		print ("Magnetometer y", unpack_data[10])
		print ("Magnetometer z", unpack_data[11])
	#   print ("Gyroscope x", unpack_data[4])
	#   print ("Gyroscope y", unpack_data[5])
	#   print ("Gyroscope z", unpack_data[6])
	#   print ("Accelerometer x", unpack_data[7])
	#   print ("Accelerometer y", unpack_data[8])
	#   print ("Accelerometer z", unpack_data[9])
	#   print ("Magnetometer x", unpack_data[10])
	#   print ("Magnetometer y", unpack_data[11])
	#   print ("Magnetometer z", unpack_data[12])

		print ("Status", unpack_data[12])
		print ("crc", unpack_data[13])

		DA_f1.write(";".join([str(x).replace(".", ",") for x in unpack_data]) + "\n")
		DA_f1.flush()
		buf = buf[DA0_SIZE:]  

	elif flug_cond[0] == flug_DA_1:
		DA1_SIZE = 24
		if len(buf) < DA1_SIZE:
			print("==== Пакет тип ДА 2 ==== Ошибка длины")
			buf = buf[1:]
			continue

		crc_cond = crc16(buf[:DA1_SIZE - 2])
		crc = struct.unpack("H", buf[DA1_SIZE - 2 : DA1_SIZE])
		if crc_cond != crc[0]:
			print("==== Пакет тип ДА 2 ==== Ошибка CRC")
			#buf = buf[1:]
			#continue

		print("==== Пакет тип ДА 2 ====")
		unpack_data = struct.unpack("<BHI3fB2H", buf[:DA1_SIZE])
		if addr_udp is not None:
			udp_socket.sendto(buf[:DA1_SIZE], addr_udp)
		print ("flug", unpack_data[0])
		print ("Number", unpack_data[1])
		print ("Time_ms", unpack_data[2])

		print ("GPS_lat", unpack_data[3])
		print ("GPS_lon", unpack_data[4])
		print ("GPS_height", unpack_data[5])
		print ("GPS_fix", unpack_data[6])
		
		print ("Photo", unpack_data[7])
		print ("crc", unpack_data[8])

		DA_f2.write(";".join([str(x).replace(".", ",") for x in unpack_data]) + "\n")
		DA_f2.flush()
		buf = buf[DA1_SIZE:]
			
	elif flug_cond[0] == flug_DA_2:
		DA2_SIZE = 28
		if len(buf) < DA2_SIZE:
			print("==== Пакет тип ДА 3 ==== Ошибка длины")
			buf = buf[1:]
			continue

		crc_cond = crc16(buf[:22])
		crc = struct.unpack("H", buf[DA2_SIZE - 2 : DA2_SIZE])
		if crc_cond != crc[0]:
			print("==== Пакет тип ДА 3 ==== Ошибка CRC")
			#buf = buf[1:]
			#continue

		print("==== Пакет тип ДА 3 ====")
		unpack_data = list(struct.unpack("<BH3I2h3HBH", buf[:DA2_SIZE]))
		if addr_udp is not None:
			udp_socket.sendto(buf[:DA2_SIZE], addr_udp)
		print ("flug", unpack_data[0])
		print ("Number", unpack_data[1])
		print ("Time_ms", unpack_data[2])	
		
		print ("Bme_press_1", unpack_data[3])
		print ("Bme_press_2", unpack_data[4])
		for i in range(2):
			unpack_data[5 + i] = unpack_data[5 + i]/100
		print ("Bme_temp_1", unpack_data[5])
		print ("Bme_temp_2", unpack_data[6])
		for i in range(2):
			unpack_data[7 + i] = unpack_data[7 + i]/10 
		print ("Bme_humidity_1", unpack_data[7])
		print ("Bme_humidity_2", unpack_data[8])
		print ("Bme_height", unpack_data[9])

		print ("Pito_speed", unpack_data[10])
		print ("crc", unpack_data[11])

		DA_f3.write(";".join([str(x).replace(".", ",") for x in unpack_data]) + "\n")
		DA_f3.flush()
	  #  print ("crc_ground", crc16(buf[:51]))
		buf = buf[DA2_SIZE:]

	else:
		buf = buf[1:]

		
f.close()
DA_f1.close()
DA_f2.close()
DA_f3.close()
MA.close()