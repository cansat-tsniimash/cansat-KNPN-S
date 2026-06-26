/*
 * appmain.c
 *
 *  Created on: 14 февр. 2026 г.
 *      Author: #СНЯЛЦЕПИ
 *
 */






#include "nRF24L01_PL/nrf24_lower_api_stm32.h"
#include "nRF24L01_PL/nrf24_upper_api.h"
#include "neo6mv2/neo6mv2.h"
#include "bmp280/bme280.h"
#include "bmp280/bmp280.h"
#include "delay/dwt_delay.h"
#include "lsm6ds3/lsm6ds3.h"
#include "lsm6ds3/lsm6ds3_reg.h"
#include "lis3mdl/lis3mdl.h"
#include "lis3mdl/lis3mdl_reg.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "math.h"
#include "fotoresictor/phororesistor.h"


#define BMP280_ADDR (0x76 << 1)

extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi2;
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim3;

#pragma pack(push, 1)
typedef struct
{

	uint8_t start;
	uint16_t number_packet;
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
	uint8_t state;
	uint16_t checksum_knpn;
	uint8_t reserv[4];

}packet_1_t;
typedef struct
{
	uint8_t start;
	uint16_t number_packet;
	uint32_t time;
	float neo6mv2_latitude;
	float neo6mv2_longitude;
	float neo6mv2_height;
	uint8_t neo6mv2_fix;
	uint16_t photoresistor;
	uint16_t checksum_knpn;
	uint8_t reserv[8];

}packet_2_t;



typedef struct
{
	uint8_t start;
	uint16_t number_packet;
	uint32_t time;
	uint32_t press1BMP280;
	uint32_t press2BMP280;
	int16_t temp1_bmp280;
	int16_t temp2_bmp280;
	uint16_t hum1_bmp280;
	uint16_t hum2_bmp280;
	uint16_t alt;
	uint8_t speed;
	uint16_t checksum_knpn;
	uint8_t reserv[4];


}packet_3_t;
#pragma pack(pop)








typedef enum
{
	NRF_STATE_PACK1,
	NRF_STATE_PACK2,
	NRF_STATE_PACK3,
	NRF_STATE_WAIT,
} nrf_state_t;

typedef enum
{
	PREPARATION,
	PACKING,
	BEFOR_UNDOCKING,
	ACCELERATION,
	FLIGHT,
	DESCEND_SAS_MODE,
	RETURN_TO_GROUND,
} glider_state_t;

#define SERVO_ANG_0 (25)
#define SERVO_ANG_180 (125)

float Tick_To_Angle(int tick){

	if (tick < SERVO_ANG_0)
	{
		return 0;
	}

	if (tick > SERVO_ANG_180)
	{
		return 180;
	}


	return (tick - SERVO_ANG_0) * 180.0/(SERVO_ANG_180 - SERVO_ANG_0);

}
int Angle_To_Tick(float angle){

	if (angle < 0)
	{
		return SERVO_ANG_0;
	}

	if (angle > 180)
	{
		return SERVO_ANG_180;
	}

	return SERVO_ANG_0 + (int)(angle * (SERVO_ANG_180 - SERVO_ANG_0)/180.0);

}
void Set_Angle(int angle){

	int tick = Angle_To_Tick(angle);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, tick);

}

void Glider_Angle(float angle){


	float result = 0.85 * angle + (0); //слишком умная штука тут вроде преобразование типов
	uint16_t tick = Angle_To_Tick(result);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, tick);
}

uint16_t checksum_knpnD(uint8_t *buf, uint16_t len){
	uint16_t checksumD = 0xFFFF;
	while (len--){
		checksumD ^= *buf++ << 8;
		for (uint8_t i = 0; i < 8; i++)
			checksumD = checksumD & 0x8000 ?(checksumD << 1) ^ 0x1021 : checksumD << 1;
	}
	return checksumD;
}

void appmain()
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

	dwt_delay_init();


	neo6mv2_Init();
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_ERR);


	HAL_Delay(200);
	nrf24_lower_api_config_t nrf24;
	nrf24_spi_pins_t pins;
	pins.ce_pin = GPIO_PIN_2;
	pins.ce_port = GPIOA;
	pins.cs_pin = GPIO_PIN_3;
	pins.cs_port = GPIOA;
	nrf24_spi_init(&nrf24, &hspi1, &pins);

	nrf24_mode_power_down(&nrf24);

	nrf24_mode_standby(&nrf24);

	nrf24_rf_config_t nrf24_conf;
	nrf24_conf.data_rate = NRF24_DATARATE_1000_KBIT;
	nrf24_conf.tx_power = NRF24_TXPOWER_MINUS_6_DBM;
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

	nrf24_fifo_status_t status_rx;
	nrf24_fifo_status_t status_tx;

	nrf24_pipe_config_t nrf24_pipe_st;
    nrf24_pipe_st.address = 0x0303030303;
    nrf24_pipe_st.payload_size = 32;
    nrf24_pipe_st.enable_auto_ack = true;
    nrf24_pipe_rx_start (&nrf24, 1 , &nrf24_pipe_st);

	nrf24_pipe_set_tx_addr(&nrf24, 0x0303030303);

	nrf24_mode_tx(&nrf24);

	packet_1_t packet1 = {0};
	packet1.start = 0xBB; // Флаг пакета

	packet_2_t packet2 = {0};
	packet2.start = 0xCC;

	packet_3_t packet3 = {0};
	packet3.start = 0xDD;

	HAL_ADC_Init(&hadc1);
	uint16_t first_foto = 0;
	float result;
	HAL_ADC_Start(&hadc1);
	HAL_ADC_GetValue(&hadc1);
	first_foto = HAL_ADC_GetValue(&hadc1);//megalux(&hadc1, &result);

	glider_state_t state = BEFOR_UNDOCKING;
	nrf_state_t nrf_state = NRF_STATE_PACK2;
	int irq;

	struct bme280_dev bmp280_1;
	bmp280_1.intf = BME280_I2C_INTF;
	bmp280_1.read = bmp280_read_reg;
	bmp280_1.write = bmp280_write_reg;
	bmp280_1.delay_us = bmp280_delay_us;


	bmp280_1.settings.osr_p = BME280_OVERSAMPLING_16X;
	bmp280_1.settings.osr_h = BME280_OVERSAMPLING_16X;
	bmp280_1.settings.osr_t = BME280_OVERSAMPLING_16X;
	bmp280_1.settings.filter = BME280_FILTER_COEFF_16;
	bmp280_1.settings.standby_time = BME280_STANDBY_TIME_20_MS;

	bmp280_bus_t bmp_bus1;
	bmp_bus1.ADDR = 0x76 << 1;
	bmp_bus1.hi2c1 = &hi2c1;
	bmp280_1.intf_ptr = &bmp_bus1;

	bme280_init(&bmp280_1);
	bme280_set_sensor_settings(BME280_ALL_SETTINGS_SEL, &bmp280_1);
	bme280_set_sensor_mode(BME280_NORMAL_MODE, &bmp280_1);

	struct bme280_data bmp_data1;


	struct bme280_dev bmp280_2;
	bmp280_2.intf = BME280_I2C_INTF;
	bmp280_2.read = bmp280_read_reg;
	bmp280_2.write = bmp280_write_reg;
	bmp280_2.delay_us = bmp280_delay_us;


	bmp280_2.settings.osr_p = BME280_OVERSAMPLING_16X;
	bmp280_2.settings.osr_h = BME280_OVERSAMPLING_16X;
	bmp280_2.settings.osr_t = BME280_OVERSAMPLING_16X;
	bmp280_2.settings.filter = BME280_FILTER_COEFF_16;
	bmp280_2.settings.standby_time = BME280_STANDBY_TIME_20_MS;

	bmp280_bus_t bmp_bus2;
	bmp_bus2.ADDR = 0x77 << 1;
	bmp_bus2.hi2c1 = &hi2c1;
	bmp280_2.intf_ptr = &bmp_bus2;

	HAL_Delay(1000);
	bme280_init(&bmp280_2);
	bme280_set_sensor_settings(BME280_ALL_SETTINGS_SEL, &bmp280_2);
	bme280_set_sensor_mode(BME280_NORMAL_MODE, &bmp280_2);

	struct bme280_data bmp_data2;

	lsm_data_t lsm_bus;
	lsm_bus.ADDR = 0b1101010 << 1;
	lsm_bus.hi2c = &hi2c1;

	stmdev_ctx_t lsm_cxt;
	lsm_cxt.handle = &lsm_bus;
	lsm_cxt.read_reg = lsm_read_reg;
	lsm_cxt.write_reg = lsm_write_reg;

	lsm6ds3_reset_set(&lsm_cxt, 1);


	lsm6ds3_xl_full_scale_set(&lsm_cxt, LSM6DS3_16g);
	lsm6ds3_xl_data_rate_set(&lsm_cxt, LSM6DS3_XL_ODR_104Hz);

	lsm6ds3_gy_full_scale_set(&lsm_cxt, LSM6DS3_2000dps);
	lsm6ds3_gy_data_rate_set(&lsm_cxt, LSM6DS3_GY_ODR_208Hz);

	int16_t bf_lsm_gy[3]= {0};
	int16_t bf_lsm_xl[3]= {0};


	int16_t temp_magn[3] = {0};


	lis3mdl_data_t lis_bus;
	lis_bus.addr =  0b0011100 << 1;
	lis_bus.hi2c1 = &hi2c1;


	stmdev_ctx_t lis;
	lis.handle = &lis_bus;
	lis.read_reg = lis3mdl_read;
	lis.write_reg = lis3mdl_write;
	lis3mdl_reset_set(&lis, 1);
	lis3mdl_operating_mode_set(&lis, LIS3MDL_CONTINUOUS_MODE);
	lis3mdl_data_rate_set(&lis, LIS3MDL_UHP_80Hz);
	lis3mdl_full_scale_set(&lis, LIS3MDL_16_GAUSS);

	FATFS fleska;
	FIL packet_file;
	HAL_Delay(1);
	char packet_path[] = "paket.bin";
	FRESULT result_mount = f_mount(&fleska, "", 1);
	FRESULT result_packet = 255;
	UINT byte_count;

	HAL_Delay(1000);

	bme280_get_sensor_data(BME280_ALL, &bmp_data1, &bmp280_1);
	float first_pres = bmp_data1.pressure;
	if (first_pres < 90000)
	{
		bme280_get_sensor_data(BME280_ALL, &bmp_data1, &bmp280_1);
		first_pres = bmp_data1.pressure;
	}

	uint16_t photo = 0;

	uint16_t res_foto;

	uint8_t Speed = 0;

	uint32_t timeOJ;
	uint32_t timecyc = 0;
	uint32_t timecyc2 = 0;







	state = PREPARATION;

	bme280_get_sensor_data(BME280_ALL, &bmp_data1, &bmp280_1);
	first_pres = bmp_data1.pressure;


	uint32_t packet_wait_deadline = 0;
	const uint32_t packet_wait_period = 5;
	while(1)
	{
		timecyc = HAL_GetTick();
		bme280_get_sensor_data(BME280_ALL, &bmp_data1, &bmp280_1);
		packet3.press1BMP280 = bmp_data1.pressure;
		packet3.hum1_bmp280 = bmp_data1.humidity*10;
		packet3.temp1_bmp280 = bmp_data1.temperature * 100;
		bme280_get_sensor_data(BME280_ALL, &bmp_data2, &bmp280_2);
		packet3.hum2_bmp280 = bmp_data2.humidity*10;
		packet3.press2BMP280 = bmp_data2.pressure;
		packet3.temp2_bmp280 = bmp_data2.temperature * 100;

		float altitude = 44330.0*(1 - pow((float)bmp_data1.pressure/first_pres, (1.0/5.255)));
		packet3.alt = altitude;


		HAL_ADC_Start(&hadc1);
		HAL_ADC_Init(&hadc1);
		photo = HAL_ADC_GetValue(&hadc1);
		packet2.photoresistor = photo;



		lsm6ds3_acceleration_raw_get(&lsm_cxt, bf_lsm_xl);
		packet1.acceleration_x = bf_lsm_xl[0];
		packet1.acceleration_y = bf_lsm_xl[1];
		packet1.acceleration_z = bf_lsm_xl[2];
		lsm6ds3_angular_rate_raw_get(&lsm_cxt, bf_lsm_gy);
		packet1.angular_x = bf_lsm_gy[0];
		packet1.angular_y = bf_lsm_gy[1];
		packet1.angular_z = bf_lsm_gy[2];
		lis3mdl_magnetic_raw_get(&lis, temp_magn);
		packet1.lis3mdl_x = temp_magn[0];
		packet1.lis3mdl_y = temp_magn[1];
		packet1.lis3mdl_z = temp_magn[2];

		Speed = sqrt(2*(bmp_data2.pressure - bmp_data1.pressure)/ 1.246);
		packet3.speed = Speed;

		GPS_Data gps_data = neo6mv2_GetData();
		packet2.neo6mv2_latitude = gps_data.latitude;
		packet2.neo6mv2_longitude = gps_data.longitude;
		packet2.neo6mv2_height = gps_data.altitude;
		packet2.neo6mv2_fix = gps_data.fixQuality;





		packet1.number_packet += 1;
		packet2.number_packet += 1;
		packet3.number_packet += 1;
		uint32_t time_pac = HAL_GetTick();
		packet1.time = time_pac;
		packet2.time = time_pac;
		packet3.time = time_pac;



		if (result_mount != FR_OK)
		{
			f_mount(NULL, "", 1);
			extern Disk_drvTypeDef disk;
			disk.is_initialized[0] = 0;
			result_mount = f_mount(&fleska, "", 1);
		}

		if (result_mount == FR_OK && result_packet != FR_OK)
		{
			if (result_packet != 255)
				f_close(&packet_file);
			result_packet = f_open(&packet_file, packet_path , FA_WRITE | FA_OPEN_ALWAYS | FA__WRITTEN);
			if(result_packet != FR_OK)
			{
				f_mount(NULL, "", 1);
				result_mount = f_mount(&fleska, "", 1);
			}

		}
		if (result_packet == FR_OK && result_mount == FR_OK)
		{
			result_packet = f_write(&packet_file, &packet1, sizeof(packet_1_t), &byte_count);
			result_packet = f_write(&packet_file, &packet2, sizeof(packet_2_t), &byte_count);
			result_packet = f_write(&packet_file, &packet3, sizeof(packet_3_t), &byte_count);
			result_packet = f_sync(&packet_file);
		}
		timecyc2 = HAL_GetTick();
		packet1.checksum_knpn = checksum_knpnD((uint8_t *) &packet1, sizeof(packet_1_t) - 2 - 4);
		packet2.checksum_knpn = checksum_knpnD((uint8_t *) &packet2, sizeof(packet_2_t) - 2 - 8);
		packet3.checksum_knpn = checksum_knpnD((uint8_t *) &packet3, sizeof(packet_3_t) - 2 - 4);

		const uint32_t now = HAL_GetTick();
		switch(nrf_state)
		{
		case NRF_STATE_PACK1:
			if (now < packet_wait_deadline)
				break;

			nrf24_fifo_status(&nrf24, &status_rx, &status_tx);
			if (status_tx != NRF24_FIFO_EMPTY)
			{
				nrf24_fifo_flush_rx(&nrf24);
				nrf24_fifo_flush_tx(&nrf24);
				nrf24_irq_clear(&nrf24, 0x07);
			}
			HAL_Delay(1);
			nrf24_fifo_write(&nrf24, (uint8_t *)&packet1, 32, false);
			HAL_Delay(1);
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
			nrf_state = NRF_STATE_PACK2;
			packet_wait_deadline = now + packet_wait_period;
			break;

		case NRF_STATE_PACK2:
			if (now < packet_wait_deadline)
				break;

			nrf24_fifo_status(&nrf24, &status_rx, &status_tx);
			if (status_tx != NRF24_FIFO_EMPTY)
			{
				nrf24_fifo_flush_rx(&nrf24);
				nrf24_fifo_flush_tx(&nrf24);
				nrf24_irq_clear(&nrf24, 0x07);
			}
			HAL_Delay(1);
			nrf24_fifo_write(&nrf24, (uint8_t *)&packet2, 32, false);
			HAL_Delay(1);
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
			nrf_state = NRF_STATE_PACK3;
			packet_wait_deadline = now + packet_wait_period;
			break;

		case NRF_STATE_PACK3:
			if (now < packet_wait_deadline)
				break;

			nrf24_fifo_status(&nrf24, &status_rx, &status_tx);
			if (status_tx != NRF24_FIFO_EMPTY)
			{
				nrf24_fifo_flush_rx(&nrf24);
				nrf24_fifo_flush_tx(&nrf24);
				nrf24_irq_clear(&nrf24, 0x07);
			}
			HAL_Delay(1);
			nrf24_fifo_write(&nrf24, (uint8_t *)&packet3, 32, false);
			HAL_Delay(1);
			HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
			nrf_state = NRF_STATE_PACK1;
			packet_wait_deadline = now + packet_wait_period;
			break;

		case NRF_STATE_WAIT:
			break;
		}

		int i = 0;
		for (i = 0; i < 8 ; i++)
		{
			nrf24_irq_get(&nrf24, &irq);
			if (irq == 0)
				break;
			nrf24_irq_clear(&nrf24, 0x07);
			HAL_Delay(1);
		}
		if (i != 0)
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

		packet1.state = state;


		switch(state)
		{
			case PREPARATION:
			{
				if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10)== GPIO_PIN_SET)
				{
					state = PACKING;
					timeOJ = HAL_GetTick();
				}
				break;
			}

			case PACKING:
			{
				if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10)== GPIO_PIN_RESET)
				{
					state = PREPARATION;
				}
				if (timeOJ + 5000 < HAL_GetTick())
				{
					state = BEFOR_UNDOCKING;
				}
				break;
			}

			case BEFOR_UNDOCKING:
			{
				if (photo > 100)
				{
					state = ACCELERATION;
					timeOJ = HAL_GetTick();
				}
				break;
			}

			case ACCELERATION:
			{
				if(timeOJ + 3000 < HAL_GetTick())
				{
					Glider_Angle(180);
					state = FLIGHT;
					timeOJ = HAL_GetTick();

				}
				break;
			}
			case FLIGHT:
			{

				if (timeOJ + 1000 < HAL_GetTick())
				{
					state = DESCEND_SAS_MODE;
				}
				break;
			}

			case DESCEND_SAS_MODE:
			{
				if (altitude < 5)
				{
					timeOJ = HAL_GetTick();
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
					state = RETURN_TO_GROUND;

				}
				break;
			}

			case RETURN_TO_GROUND:
			{
				if (timeOJ + 1000 < HAL_GetTick())
				{
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
				}

				if (timeOJ + 2000 < HAL_GetTick())
				{
					HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9);
					timeOJ = HAL_GetTick();
				}
				break;
			}


		}


		for (int i = 0; i < 50; i++)
		{
			if (neo6mv2_work())
			{
				break;
			}
		}

	}






}
