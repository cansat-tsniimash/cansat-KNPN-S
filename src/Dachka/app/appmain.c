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



extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

#pragma pack(push, 1)
typedef struct
{
	uint16_t start;
	uint16_t number_packet;
	uint32_t time;
	int16_t temp_bme280;
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
	float neo6mv2_latitude;
	float neo6mv2_longitude;
	float neo6mv2_height;
	uint8_t neo6mv2_fix;
	uint8_t checksum_knpn;




}packet_1_t;
typedef struct
{
	uint16_t start;
	uint16_t number_packet;
	uint16_t team_id;



}packet_2_t;
#pragma pack(pop)




void appmain()
{

	uint8_t nn[1000] = {0};
	HAL_UART_Receive(&huart1, nn, 1000, 1000);

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
	nrf24_prot.auto_retransmit_delay = 01;
	nrf24_prot.auto_retransmit_count = 1;
	nrf24_setup_protocol(&nrf24, &nrf24_prot);

	nrf24_pipe_config_t nrf24_pipe_st;
	nrf24_pipe_st.address = 3;
	nrf24_pipe_st.payload_size = 0;
	nrf24_pipe_st.enable_auto_ack = false;
	nrf24_pipe_rx_start (&nrf24, 1 , &nrf24_pipe_st);
	nrf24_pipe_rx_stop (&nrf24, 1);

	nrf24_fifo_status_t status_rx[32] = {0};
	nrf24_fifo_status_t status_tx[32] = {0};

	nrf24_pipe_set_tx_addr(&nrf24, 0x03 );

	nrf24_mode_standby(&nrf24);

	nrf24_mode_rx(&nrf24);

	packet_1_t packet1 = {0};
	packet1.start = 0xAAAA; // Флаг пакета









	while(1)
	{







		/*nrf24_fifo_status(&nrf24, status_rx, status_tx);

		if (nrf24_fifo_status > 0)
		{
			nrf24_fifo_flush_rx(&nrf24);
			nrf24_fifo_flush_tx(&nrf24);
		}
		if (nrf24_fifo_status == 0)
		{

			nrf24_fifo_read(&nrf24, packet1 , 32);

		}
		if (nrf24_fifo_read == 0)
		{
			nrf24_fifo_flush_rx(&nrf24);
			nrf24_fifo_flush_tx(&nrf24);
		}*/


		for (int i = 0; i < 50; i++)
		{
			if (neo6mv2_work())
			{
				break;
			}
		}
		GPS_Data gps_data = neo6mv2_GetData();
		packet1.neo6mv2_latitude = gps_data.latitude;
		packet1.neo6mv2_longitude = gps_data.longitude;
		packet1.neo6mv2_height = gps_data.altitude;
		packet1.neo6mv2_fix = gps_data.fixQuality;

		printf("%d Печень ", gps_data.cookie);
		printf("%f Ширина ", packet1.neo6mv2_latitude);
		printf("%f Долгота ", packet1.neo6mv2_longitude);
		printf("%f М ", packet1.neo6mv2_height);
		printf("%i спутники", gps_data.satellites);
		printf("%i\n", packet1.neo6mv2_fix);


	}






}
