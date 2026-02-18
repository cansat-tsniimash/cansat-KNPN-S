/*
 * appmain.c
 *
 *  Created on: 14 февр. 2026 г.
 *      Author: #СНЯЛЦЕПИ
 *
 */

#include "nRF24L01_PL/nrf24_lower_api_stm32.h"
#include "nRF24L01_PL/nrf24_upper_api.h"

extern SPI_HandleTypeDef hspi1;

void appmain()
{


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

	nrf24_pipe_set_tx_addr(&nrf24, 0x03 );

	nrf24_mode_standby(&nrf24);

	nrf24_mode_rx(&nrf24);








}
