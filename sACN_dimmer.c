#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ssid_config.h"

#include "multipwm.h"
#include "E131.h"

e131_packet_t pbuff; /* Packet buffer */
e131_packet_t *pwbuff; /* Pointer to working packet buffer */

void e131task(void *pvParameters) {
	printf("Open server.\r\n");
	vTaskDelay(1000);

	struct netconn *conn;
	err_t err;

	/* Create a new connection handle */
	conn = netconn_new(NETCONN_UDP);
	if(!conn) {
		printf("Error: Failed to allocate socket.\r\n");
		return;
	}

	/* Bind to port with default IP address */
	err = netconn_bind(conn, IP_ADDR_ANY, E131_DEFAULT_PORT);
	if(err != ERR_OK) {
		printf("Error: Failed to bind socket. err=%d\r\n", err);
		return;
	}

	ip4_addr_t multiaddr;
	IP4_ADDR(&multiaddr, 239, 255, 0, 1); //IPv4 local scope multicast

	err = netconn_join_leave_group(conn, &multiaddr, &netif_default->ip_addr, NETCONN_JOIN);
	if(err != ERR_OK) {
		printf("Error: Join Multicast Group. err=%d\r\n", err);
		return;
	}

	printf("Listening for connections.\r\n");

	while(1) {
		struct netbuf *buf;

		err = netconn_recv(conn, &buf);
		if(err != ERR_OK) {
			printf("Error: Failed to receive packet. err=%d\r\n", err);
			continue;
		}

		if(buf->p->tot_len == sizeof(pwbuff->raw)) {
			//If packet is 638 bytes we handle it as a correct package and copy it to a global struct
			if(netbuf_copy(buf, pwbuff->raw, sizeof(pwbuff->raw)) != buf->p->tot_len) {
				printf("Error: Couldn't copy buffer. err=%d\r\n", err);
			}
		} else {
			printf("Wrong packet size.\n\n");
		}

		netbuf_delete(buf);
	}
}

void pwmtask(void *pvParameters)
{
	//Set up PWM for pins
	uint8_t    pins[] = {14, 12, 13, 15, 3}; //NodeMCU D5-D9 https://github.com/nodemcu/nodemcu-devkit-v1.0#pin-map
	pwm_info_t pwm_info;
	pwm_info.channels = sizeof(pins);
	pwm_info.reverse = true;
	multipwm_init(&pwm_info);
	for (uint8_t ii=0; ii<pwm_info.channels; ii++) {
		multipwm_set_pin(&pwm_info, ii, pins[ii]);
	}

    while(1) {
		multipwm_stop(&pwm_info);

		uint16_t channel[pwm_info.channels];
		for (uint8_t i=0; i<pwm_info.channels; i++) {
			channel[i] = pwbuff->property_values[i+1]; //Get DMX channel value from sACN struct
			//Upscale 8 bit DMX value to 16 bit, and add original value to fit the range from 0-65535
			channel[i] = (channel[i] << 8) + channel[i];
			multipwm_set_duty(&pwm_info, i, channel[i]);
		}

		//Only print values once every modula
		if ((rand() % 50) == 0) {
			for (uint8_t i=0; i<pwm_info.channels; i++) {
				printf("Channel %d: %d\n", i+1, channel[i]);
			}
		}

		multipwm_start(&pwm_info);
		vTaskDelay(1);
	}
}

void user_init(void) {
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

	memset(pbuff.raw, 0, sizeof(pbuff.raw));
	pwbuff = &pbuff;
	xTaskCreate(e131task, "e131task", 768, NULL, 8, NULL);
	xTaskCreate(pwmtask, "pwmtask", 256, NULL, 2, NULL);
}

