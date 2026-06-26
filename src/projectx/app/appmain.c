
#include <stm32f4xx.h>

// БАЗОВЫЕ БИБЛИОТЕКИ

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// INCLUDE ДАТЧИКОВ

#include "bmp280/bmp.h"
#include "lis3mdl\lis3mdl.h"
#include "lsm6ds3\lsm6ds3.h"

#include "resistor\resistor.h"

#include "fatfs.h"


#include "e220400t22s/e220_400t22s.h" // радио (UART)
#include "nRF24L01_PL/nrf24_upper_api.h"
#include "nRF24L01_PL/nrf24_lower_api.h"
#include "nRF24L01_PL/nrf24_lower_api_stm32.h"

#include "neo6mv2\neo6mv2.h"

#include "dwt_delay.h"

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi1;



#pragma pack(push, 1)
typedef struct{
	uint8_t start;
	uint16_t number_packet;
	uint16_t team_id;
	uint32_t time;
	int16_t angular_x;
	int16_t angular_y;
	int16_t angular_z;
	int16_t acceleration_x;
	int16_t acceleration_y;
	int16_t acceleration_z;
	int16_t lis3mdl_x;
	int16_t lis3mdl_y;
	int16_t lis3mdl_z;
	uint32_t pressure_bme280;
	int16_t temp_bme280;
	uint16_t humidity_bme280;
	uint16_t altitude_bme280;
	float neo6mv2_latitude;
	float neo6mv2_longitude;
	float neo6mv2_height;
	uint8_t neo6mv2_fix;
	uint16_t photoresistor;
	uint8_t state;
	uint16_t checksum_knpn;
} packet_t;

#pragma pack(pop)
typedef enum
{
	PREPARATION,
	PACKING,
	IN_ROCKET,
	DESCEND_MODE,
	UNDOCKING,
	RETURN_TO_GROUND,
	TOUCHING_THE_GROUND,
} mother_state_t;


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


uint16_t checksum_knpn(uint8_t *buf, uint16_t len){
	uint16_t checksum = 0xFFFF;
	while (len--){
		checksum ^= *buf++ << 8;
		for (uint8_t i = 0; i < 8; i++)
			checksum = checksum & 0x8000 ?(checksum << 1) ^ 0x1021 : checksum << 1;
	}
	return checksum;
}

void appmain(){

	int i;


	packet_t packet = {0};
	packet.team_id = 0xD9;
	packet.start = 0xAA;
	packet.number_packet = 0;


	// bme280
	struct bme280_data data;
	bme280_dev_t bme = bme_init();


	// LSM6DS3
	int16_t temp_gyro[3] = {0};
	int16_t temp_accel[3] = {0};
	stmdev_ctx_t lsm;
	lsm_init(&lsm, &hi2c1);

	// LIS3MDL
	int16_t temp_magn[3] = {0};
	stmdev_ctx_t lis;
	lis3mdl_init(&lis, &hi2c1);


	// sd
	FATFS fileSystem;
	FIL binFile;

    UINT testBytes;
    FRESULT mount_res = 255;
    FRESULT bin_res = 255;
    uint8_t bin_path[] = "knpn.bin\0";


    //NRF24
    nrf24_lower_api_config_t nrf24;
    nrf24_spi_pins_t pins;
    pins.ce_pin = GPIO_PIN_12;
    pins.ce_port = GPIOB;
    pins.cs_pin = GPIO_PIN_13;
    pins.cs_port = GPIOB;
    nrf24_spi_init(&nrf24, &hspi1, &pins);

    nrf24_mode_power_down(&nrf24);
    nrf24_mode_standby(&nrf24);

    nrf24_rf_config_t nrf24_conf;
    nrf24_conf.data_rate = NRF24_DATARATE_1000_KBIT;
    nrf24_conf.tx_power = NRF24_TXPOWER_MINUS_0_DBM;
    nrf24_conf.rf_channel = 0;
    nrf24_setup_rf(&nrf24, &nrf24_conf);

    nrf24_protocol_config_t nrf24_prot;
    nrf24_prot.crc_size = NRF24_CRCSIZE_2BYTE;
    nrf24_prot.address_width = NRF24_ADDRES_WIDTH_5_BYTES;
    nrf24_prot.en_dyn_payload_size = true;
    nrf24_prot.en_ack_payload = true;
    nrf24_prot.en_dyn_ack = true;
    nrf24_prot.auto_retransmit_delay = 2;
    nrf24_prot.auto_retransmit_count = 15;
    nrf24_setup_protocol(&nrf24, &nrf24_prot);

    nrf24_pipe_config_t nrf24_pipe_st;
    nrf24_pipe_st.address = 0x0303030303;
    nrf24_pipe_st.payload_size = 32;
    nrf24_pipe_st.enable_auto_ack = true;
    nrf24_pipe_rx_start (&nrf24, 0, &nrf24_pipe_st);

    nrf24_pipe_set_tx_addr(&nrf24, 0x0303030303);

    nrf24_fifo_status_t status_rx;
    nrf24_fifo_status_t status_tx;


    nrf24_mode_rx(&nrf24);

    uint8_t bufdoc[3][32]={0};

    mother_state_t state = PREPARATION;

    neo6mv2_Init();
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_ERR);

	// e220-400t22s
	e220_pins_t e220_bus;
	e220_bus.m0_pinchik = GPIO_PIN_0;
	e220_bus.m1_pinchik = GPIO_PIN_1;
	e220_bus.m0_port = GPIOA;
    e220_bus.m1_port = GPIOA;
    e220_bus.aux_pin = GPIO_PIN_4;
	e220_bus.aux_port = GPIOA;
    e220_bus.uart = &huart2;
    e220_set_mode(e220_bus, E220_MODE_DSM);


    HAL_Delay(100);
    e220_set_addr(e220_bus, 0xFFFF);
    HAL_Delay(100);
    e220_set_reg1(e220_bus, E220_REG1_PACKET_LEN_200B, E220_REG1_RSSI_OFF, E220_REG1_TPOWER_22);
    HAL_Delay(100);
    e220_set_channel(e220_bus, 43);
    HAL_Delay(100);
    e220_set_reg3(e220_bus, E220_REG3_RSSI_BYTE_OFF, E220_REG3_TRANS_M_TRANSPARENT, E220_REG3_LBT_EN_OFF, E220_REG3_WOR_CYCLE_500);
    HAL_Delay(100);
    e220_set_reg0(e220_bus, E220_REG0_AIR_RATE_19200, E220_REG0_PARITY_8N1_DEF, E220_REG0_PORT_RATE_115200);
    HAL_Delay(100);

    huart2.Init.BaudRate = 115000;
    HAL_UART_Init(&huart2);

    e220_set_mode(e220_bus, E220_MODE_TM);
    uint8_t nrf_num = 0;
    uint16_t nrf_count = 0;

    int irq;
    nrf24_fifo_flush_rx(&nrf24);
    nrf24_flush_rx(&nrf24);

    bme280_get_sensor_data(BME280_ALL, &data, &bme);
    float first_pres = data.pressure;


    uint16_t foto;

    float result;

    uint16_t first_foto;
    first_foto = megalux(&hadc1, &result);
    uint32_t timeOJ;
    timeOJ = HAL_GetTick();


	while(1){


		megalux(&hadc1, &result);

		packet.photoresistor = result;
		foto = packet.photoresistor;


		nrf24_fifo_status(&nrf24, &status_rx, &status_tx);
		if (status_rx != NRF24_FIFO_EMPTY)
		{
			nrf_num = nrf24_fifo_read(&nrf24, bufdoc[0], 32);
			if (nrf_num == 0)
			{
				nrf24_fifo_flush_rx(&nrf24);
				nrf24_fifo_status(&nrf24, &status_rx, &status_tx);
			}
			else
			{
				nrf_count++;
				//gcs
				e220_send_packet(e220_bus, (uint8_t *)&bufdoc, sizeof(bufdoc));
				nrf24_irq_clear(&nrf24, 0x07);
				HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
			}

		}

		for (int i = 0; i < 4 ; i++)
		{
			nrf24_irq_get(&nrf24, &irq);
			if (irq == 0)
				break;
			nrf24_irq_clear(&nrf24, 0x07);
			HAL_Delay(1);
		}



		// bme280
		bme280_get_sensor_data(BME280_ALL, &data, &bme);
		packet.pressure_bme280 = data.pressure;
		packet.temp_bme280 = data.temperature * 100;
		packet.humidity_bme280 = data.humidity * 100;
		float altitude = 44330.0 *(1 - pow((float)data.pressure/first_pres, (1.0/5.255)));
		packet.altitude_bme280 = altitude;

		// LSM6DS3
		lsm6ds3_angular_rate_raw_get(&lsm, temp_gyro);
		packet.angular_x = temp_gyro[0];
		packet.angular_y = temp_gyro[1];
		packet.angular_z = temp_gyro[2];

		lsm6ds3_acceleration_raw_get(&lsm, temp_accel);
		packet.acceleration_x = temp_accel[0];
		packet.acceleration_y = temp_accel[1];
		packet.acceleration_z = temp_accel[2];

		// LIS3MDL
		lis3mdl_magnetic_raw_get(&lis, temp_magn);
		packet.lis3mdl_x = temp_magn[0];
		packet.lis3mdl_y = temp_magn[1];
		packet.lis3mdl_z = temp_magn[2];





// Запаковка телеметрии
		packet.time = HAL_GetTick();
		packet.number_packet++;
		packet.checksum_knpn = checksum_knpn((uint8_t *)&packet, sizeof(packet_t) - 2);

		// e220-400t22s
		HAL_Delay(100);
	    e220_send_packet(e220_bus, (uint8_t *)&packet, sizeof(packet_t));


		// sd
		if (mount_res != FR_OK){
			f_mount(NULL, "", 0);
			mount_res = f_mount(&fileSystem, "", 1);
			bin_res = f_open(&binFile, (char*)bin_path, FA_WRITE | 0x30);
		}

		if  (mount_res == FR_OK && bin_res != FR_OK){
			f_close(&binFile);
			bin_res = f_open(&binFile, (char*)bin_path, FA_WRITE | 0x30);
		}

		if (mount_res == FR_OK && bin_res == FR_OK)
		{
			bin_res = f_write(&binFile, (uint8_t*)&bufdoc, sizeof(bufdoc), &testBytes);
			bin_res = f_write(&binFile, (uint8_t*)&packet, sizeof(packet_t), &testBytes);
			f_sync(&binFile);
		}

		//neo6mv2
	    for (i = 0; i < 50; i++)
	    {
	    	if (neo6mv2_work())
	    		break;
	    }
		GPS_Data gps_data = neo6mv2_GetData();
		packet.neo6mv2_latitude = gps_data.latitude;
		packet.neo6mv2_longitude = gps_data.longitude;
		packet.neo6mv2_height = gps_data.altitude;
		packet.neo6mv2_fix = (gps_data.fixQuality << 5) | (gps_data.cookie & 0x1F);

		packet.state = state;


		switch(state)
		{
			case PREPARATION:
			{
				if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_SET)
				{
					state = PACKING;
					timeOJ = HAL_GetTick();
				}
				break;
			}

			case PACKING:
			{
				if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET)
				{
					state = PREPARATION;
				}
				if (timeOJ + 5000 < HAL_GetTick())
				{
					state = IN_ROCKET;
				}
				break;
			}
		  	case IN_ROCKET:
		  	{
		  		if (foto > first_foto * 0.8)
		  		{
		  			state = DESCEND_MODE;
		  		}
		  		break;
		 	}
		  	case DESCEND_MODE:
		  	{
		  		if (altitude < 700)
		  		{
		  			state = UNDOCKING;
		  			timeOJ = HAL_GetTick();
		  			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
		  		}
		  		break;
		 	}
		  	case UNDOCKING:
		  	{

		  		if (timeOJ + 1000 < HAL_GetTick())
		  		{
		  			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
		  			state = RETURN_TO_GROUND;
		  		}
		  		break;
		  	}

		  	case RETURN_TO_GROUND:
		  	{
		  		if (altitude < 150)
		  		{
		  			state = TOUCHING_THE_GROUND;
		  			timeOJ = HAL_GetTick();
		  		}
		  		break;
		  	}
		  	case TOUCHING_THE_GROUND:
		  	{
		  		if (timeOJ + 2000 < HAL_GetTick())
		  		{
		  			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_11);
		  			timeOJ = HAL_GetTick();
		  		}
		  		break;
		  	}



		}

	}
}
