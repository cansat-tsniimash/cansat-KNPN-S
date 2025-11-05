/*
* APPMAIN ДЛЯ МА (МАТЕРИНСКИЙ АППАРАТ)
*
* НАЧАЛО РАЗРАБОТКИ 27.09.2025
*
* ПРОГРЕСС РАЗРАБОТКИ:
*
* BME280(75%) - перенес
* LIS3MDL (НЕ НАЧАТО)   - перенес
* LDM6DS3 (НЕ НАЧАТО)   - перенес
* E220400T22S - ПЕРЕНЕС
* SD-CARD
* NEO8Mv2
*
* пиво
*
*
*/

#include <stm32f4xx.h>

// БАЗОВЫЕ БИБЛИОТЕКИ

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// INCLUDE ДАТЧИКОВ

#include "bmp280/bmp.h" // датчик bme280 (I2C)
#include "lis3mdl\lis3mdl.h" // датчик lis3mdl (I2C)
#include "lsm6ds3\lsm6ds3.h" // датчик lsm6ds3 (I2C)

#include "resistor\resistor.h" // фоторезистор (ADC)

#include "fatfs_sd\fatfs_sd.h" // micro sd (SPI)
#include "..\Middlewares\Third_Party\FatFs\src\ff.h" // micro sd (SPI)

#include "e220400t22s/e220_400t22s.h" // радио (UART)

#include "dwt_delay.h" // тайминги

// ОБРАБОТЧИКИ
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;

// Вычисление контрольной суммы массива байтов
uint8_t xorBlock(const uint8_t *data, size_t size) {
	uint8_t result = 0x00;

	for (size_t i = 0; i < size; i++) {
		result ^= data[i];
	}

	return result;
}

// структура для хранения данных с датчиков
typedef struct{
	uint8_t accel_error;
	uint8_t gyro_error;
	uint8_t magn_error;
	uint8_t lis_err;
	uint8_t lsm_err;
} data_t;

// структура для хранения и передачи телеметрии
#pragma pack(push, 1) // Обращение к компилятору не выравнивать структуру и хранить её в памяти без пустых байтов
typedef struct{
	uint16_t start;
	uint16_t number_packet;
	uint16_t team_id;
	uint32_t time;
	int16_t temp_bme280;
	uint32_t altitude_bme280;
	uint32_t pressure_bme280;
	int16_t humidity_bme280;
	int16_t acceleration_x;
	int16_t acceleration_y;
	int16_t acceleration_z;
	int16_t angular_x;
	int16_t angular_y;
	int16_t angular_z;
	uint8_t state;
	uint16_t photoresistor;
	int16_t lis3mdl_x;
	int16_t lis3mdl_y;
	int16_t lis3mdl_z;
	int16_t ds18b20;
	float neo8mv2_latitude;
	float neo8mv2_longitude;
	float neo8mv2_height;
	uint8_t neo8mv2_fix;
	uint8_t checksum_knpn;
} packet_t;
#pragma pack(pop) // Компилятор может добавлять выравнивающие байты для оптимизации работы процессора

bme280_dev_t bme_init(){
	bme280_dev_t bme;
	bme.delay_us = bmp_delay;
	bme.settings.filter = BME280_FILTER_COEFF_2;
	bme.settings.osr_h = BME280_OVERSAMPLING_16X;
	bme.settings.osr_p = BME280_OVERSAMPLING_16X;
	bme.settings.osr_t = BME280_OVERSAMPLING_16X;
	bme.settings.standby_time = BME280_STANDBY_TIME_500_MS;
	bmp_init(&bme, &hi2c1);
	return bme;
}

void appmain(){

	packet_t packet = {0};
	packet.team_id = 0xD9;
	packet.start = 0xAAAA;
	packet.number_packet = 0;
	volatile data_t my_data;
	int16_t temp_gyro[3] = {0}; // temp = ВРЕМЕННО!
	int16_t temp_accel[3] = {0};

	// bme280
	struct bme280_data data;
	bme280_dev_t bme = bme_init();


	// LSM6DS3
	stmdev_ctx_t lsm;
	my_data.lsm_err = lsm_init(&lsm, &hi2c1);

	// LIS3MDL
	stmdev_ctx_t lis;
	my_data.lis_err = lis_init(&lis, &hi2c1);
	int16_t temp_magn[3] = {0};

	// sd
	FATFS fileSystem; // переменная типа FATFS
	FIL binFile;
	//csvFile; // хендлер файла
    UINT testBytes;  // Количество записанных байт

    FRESULT mount_res = 255;
    FRESULT bin_res = 255;
    //FRESULT csv_res = 255;
    uint8_t bin_path[] = "grib.bin\0";
    //uint8_t csv_path[] = "grib.csv\0";
    //char str_buffer[300] = {0};
    //char str_header[330] = "number_packet; time; temp_bme280; pressure_bme280; acceleration x; acceleration y; acceleration z; angular x; angular y; angular z; checksum_org; state; photoresistor; lis3mdl_x; lis3mdl_y; lis3mdl_z; ds18b20; ne06mv2_height; ne06mv2_latitude; ne06mv2_longitude; ne06mv2_height; ne06mv2_fix; scd41; mq_4; me2o2; checksum_grib;\n";
    /*int mount_attemps;
    for(mount_attemps = 0; mount_attemps < 5; mount_attemps++)
    {
    	mount_res = f_mount(&fileSystem, "0:", 0);
        if (mount_res == FR_OK) {
        	res = f_open(&binFile, "gribochek_raw.bin\0", FA_WRITE | FA_CREATE_ALWAYS);
        	break;
        }
    }*/

	// e220-400t22s
	e220_pins_t e220_bus;
	e220_bus.m0_pinchik = GPIO_PIN_1;
	e220_bus.m1_pinchik = GPIO_PIN_0;
	e220_bus.m0_port = GPIOB;
    e220_bus.m1_port = GPIOB;
    e220_bus.aux_pin = GPIO_PIN_3;
	e220_bus.aux_port = GPIOB;
    e220_bus.uart = &huart2;
    e220_set_mode(e220_bus, E220_MODE_DSM);

    e220_set_addr(e220_bus, 0xFFFF);
    HAL_Delay(100);
    e220_set_reg0(e220_bus, E220_REG0_AIR_RATE_9600, E220_REG0_PARITY_8N1_DEF, E220_REG0_PORT_RATE_9600);
    HAL_Delay(100);
    e220_set_reg1(e220_bus, E220_REG1_PACKET_LEN_200B, E220_REG1_RSSI_OFF, E220_REG1_TPOWER_22);
    HAL_Delay(100);
    e220_set_channel(e220_bus, 1);
    HAL_Delay(100);
    e220_set_reg3(e220_bus, E220_REG3_RSSI_BYTE_OFF, E220_REG3_TRANS_M_TRANSPARENT, E220_REG3_LBT_EN_OFF, E220_REG3_WOR_CYCLE_500);
    e220_set_mode(e220_bus, E220_MODE_TM);

    float result;


	while(1){
		// bme280
		bme280_get_sensor_data(BME280_ALL, &data, &bme); // вывод давления и температуры
		packet.pressure_bme280 = data.pressure;
		packet.temp_bme280 = data.temperature * 100;
		packet.humidity_bme280 = data.humidity * 100;

		// LSM6DS3
		my_data.gyro_error = lsm6ds3_angular_rate_raw_get(&lsm, temp_gyro);
		packet.angular_x = temp_gyro[0];
		packet.angular_y = temp_gyro[1];
		packet.angular_z = temp_gyro[2];

		my_data.accel_error = lsm6ds3_acceleration_raw_get(&lsm, temp_accel);
		packet.acceleration_x = temp_accel[0];
		packet.acceleration_y = temp_accel[1];
		packet.acceleration_z = temp_accel[2];
		// LIS3MDL
		my_data.magn_error = lis3mdl_magnetic_raw_get(&lis, temp_magn);
		packet.lis3mdl_x = temp_magn[0];
		packet.lis3mdl_y = temp_magn[1];
		packet.lis3mdl_z = temp_magn[2];

		megalux(&hadc1, &result);
		packet.photoresistor = result * 1000;

	    if(my_data.magn_error != 0){
	    	packet.state |= 1 << 5;
	    }
	    if(my_data.lsm_err != 0){
	    	packet.state |= 1 << 6;
	    }
	    if(bme.intf_rslt != 0){
	    	packet.state |= 1 << 7;
	    }

		packet.time = HAL_GetTick();
		packet.number_packet++;
		packet.checksum_knpn = xorBlock((uint8_t *)&packet, sizeof(packet_t) - 1);

		// e220-400t22s
	    e220_send_packet(e220_bus, 0xFFFF, (uint8_t *)&packet, sizeof(packet_t), 23);

		// sd
		if (mount_res != FR_OK){
			f_mount(NULL, "", 0);
			mount_res = f_mount(&fileSystem, "", 1);
			bin_res = f_open(&binFile, (char*)bin_path, FA_WRITE | 0x30);
			//csv_res = f_open(&csvFile, (char*)csv_path, FA_WRITE | 0x30);
			//csv_res = f_write(&csvFile, str_header, 300, &testBytes);
		}

		if  (mount_res == FR_OK && bin_res != FR_OK){
			f_close(&binFile);
			bin_res = f_open(&binFile, (char*)bin_path, FA_WRITE | 0x30);
		}

		if (mount_res == FR_OK && bin_res == FR_OK)
		{
			bin_res = f_write(&binFile, (uint8_t*)&packet, sizeof(packet_t), &testBytes);
			f_sync(&binFile);
		}
	}
}
