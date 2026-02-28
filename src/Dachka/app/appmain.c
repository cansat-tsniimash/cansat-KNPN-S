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

#define BMP280_ADDR (0x76 << 1)

extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

#pragma pack(push, 1)
typedef struct
{

	uint16_t start;
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
	uint8_t checksum_knpn;
	uint8_t reserv[4];

}packet_1_t;
typedef struct
{
	uint16_t start;
	uint16_t number_packet;
	uint32_t time;
	float neo6mv2_latitude;
	float neo6mv2_longitude;
	float neo6mv2_height;
	uint8_t neo6mv2_fix;
	uint16_t photoresistor;
	uint8_t checksum_knpn;
	uint8_t reserv[8];




}packet_2_t;



typedef struct
{
	uint16_t start;
	uint16_t number_packet;
	uint32_t time;
	float press1BMP280;
	uint16_t temp1_bmp280;
	uint16_t hum1_bmp280;
	float press2BMP280;
	uint16_t temp2_bmp280;
	uint16_t hum2_bmp280;
	uint8_t speed;
	uint8_t reserv[7];


}packet_3_t;
#pragma pack(pop)








typedef enum
{
	NRF_STATE_PACK1,
	NRF_STATE_PACK2,
	NRF_STATE_PACK3,
	NRF_STATE_WAIT,
} nrf_state_t;


void appmain()
{

	dwt_delay_init();


	neo6mv2_Init();
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_ERR);





	nrf24_lower_api_config_t nrf24;
	nrf24_spi_pins_t pins;
	pins.ce_pin = GPIO_PIN_15;
	pins.ce_port = GPIOA;
	pins.cs_pin = GPIO_PIN_6;
	pins.cs_port = GPIOB;
	nrf24_spi_init(&nrf24, &hspi1, &pins);

	nrf24_mode_power_down(&nrf24);

	nrf24_rf_config_t nrf24_conf;
	nrf24_conf.data_rate = NRF24_DATARATE_250_KBIT;
	nrf24_conf.tx_power = NRF24_TXPOWER_MINUS_18_DBM;
	nrf24_conf.rf_channel = 3;
	nrf24_setup_rf(&nrf24, &nrf24_conf);

	nrf24_protocol_config_t nrf24_prot;
	nrf24_prot.crc_size = NRF24_CRCSIZE_2BYTE;
	nrf24_prot.address_width = NRF24_ADDRES_WIDTH_5_BYTES;
	nrf24_prot.en_dyn_payload_size = false;
	nrf24_prot.en_ack_payload = false;
	nrf24_prot.en_dyn_ack = false;
	nrf24_prot.auto_retransmit_delay = 1;
	nrf24_prot.auto_retransmit_count = 1;
	nrf24_setup_protocol(&nrf24, &nrf24_prot);

	nrf24_fifo_status_t status_rx;
	nrf24_fifo_status_t status_tx;

	nrf24_pipe_set_tx_addr(&nrf24, 0x03 );

	nrf24_mode_standby(&nrf24);

	nrf24_mode_tx(&nrf24);

	packet_1_t packet1 = {0};
	packet1.start = 0xAAAA; // Флаг пакета

	packet_2_t packet2 = {0};
	packet2.start = 0xAAAB;

	packet_3_t packet3 = {0};
	packet3.start = 0xAAAC;



	nrf_state_t nrf_state = NRF_STATE_PACK1;


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

	bmp280_bus_t bmp_bus;
	bmp_bus.ADDR = 0x76 << 1;
	bmp_bus.hi2c1 = &hi2c1;
	bmp280_1.intf_ptr = &bmp_bus;

	bme280_init(&bmp280_1);
	bme280_set_sensor_settings(BME280_ALL_SETTINGS_SEL, &bmp280_1);
	bme280_set_sensor_mode(BME280_NORMAL_MODE, &bmp280_1);

	struct bme280_data bmp_data;



	while(1)
	{

		bme280_get_sensor_data(BME280_ALL, &bmp_data, &bmp280_1);


		switch(nrf_state)
		{
		case NRF_STATE_PACK1:
			nrf24_fifo_status(&nrf24, &status_rx, &status_tx);
			if (status_tx == NRF24_FIFO_EMPTY)
			{
				nrf24_fifo_write(&nrf24, &packet1, 32, false);
			}
			else
			{
				nrf24_fifo_flush_rx(&nrf24);
				nrf24_fifo_flush_tx(&nrf24);
			}
			nrf_state = NRF_STATE_PACK2;
			break;

		case NRF_STATE_PACK2:
			nrf24_fifo_status(&nrf24, &status_rx, &status_tx);
			if (status_tx == NRF24_FIFO_EMPTY)
			{
				nrf24_fifo_write(&nrf24, &packet2, 32, false);
			}
			else
			{
				nrf24_fifo_flush_rx(&nrf24);
				nrf24_fifo_flush_tx(&nrf24);
			}
			nrf_state = NRF_STATE_PACK3;
			break;

		case NRF_STATE_PACK3:
			nrf24_fifo_status(&nrf24, &status_rx, &status_tx);
			if (status_tx == NRF24_FIFO_EMPTY)
			{
				nrf24_fifo_write(&nrf24, &packet3, 32, false);
			}
			else
			{
				nrf24_fifo_flush_rx(&nrf24);
				nrf24_fifo_flush_tx(&nrf24);
			}
			nrf_state = NRF_STATE_PACK1;
			break;
		}

		for (int i = 0; i < 50; i++)
		{
			if (neo6mv2_work())
			{
				break;
			}
		}
		GPS_Data gps_data = neo6mv2_GetData();
		packet2.neo6mv2_latitude = gps_data.latitude;
		packet2.neo6mv2_longitude = gps_data.longitude;
		packet2.neo6mv2_height = gps_data.altitude;
		packet2.neo6mv2_fix = gps_data.fixQuality;

		printf("%d Печень ", gps_data.cookie);
		printf("%f Ширина ", packet2.neo6mv2_latitude);
		printf("%f Долгота ", packet2.neo6mv2_longitude);
		printf("%f М ", packet2.neo6mv2_height);
		printf("%i спутники", gps_data.satellites);
		printf("%i\n", packet2.neo6mv2_fix);


	}






}
