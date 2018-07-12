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

#include "E131.h"

e131_packet_t pbuff; /* Packet buffer */
e131_packet_t *pwbuff; /* Pointer to working packet buffer */

void e131task(void *pvParameters) {
	printf("Open server in 10 seconds.\r\n");
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
	//IP4_ADDR(&multiaddr, 224, 0, 0, 0);
	IP4_ADDR(&multiaddr, 239, 255, 0, 1);

	err = netconn_join_leave_group(conn, &multiaddr, &netif_default->ip_addr, NETCONN_JOIN);
	if(err != ERR_OK) {
		printf("Error: Join Multicast Group. err=%d\r\n", err);
		return;
	}

	printf("Listening for connections.\r\n");
	memset(pbuff.raw, 0, sizeof(pbuff.raw));
	pwbuff = &pbuff;

	while(1) {
		//printf("Loop\r\n");
		struct netbuf *buf;

		err = netconn_recv(conn, &buf);
		if(err != ERR_OK) {
			printf("Error: Failed to receive packet. err=%d\r\n", err);
			continue;
		}

		//If packet is 638 bytes we handle it as a correct package and put it in
		if(buf->p->tot_len == sizeof(pwbuff->raw)) {
			if(netbuf_copy(buf, pwbuff->raw, sizeof(pwbuff->raw)) != buf->p->tot_len) {
				printf("Error: Couldn't copy buffer. err=%d\r\n", err);
			} else {
				//printf("Universe %d\n\n", pwbuff->universe);
				//printf("Seq %d\n\n", pwbuff->sequence_number);
				printf("V %d\n\n", pwbuff->property_values[1]);
			}
		} else {
			printf("Wrong packet size.\n\n");
		}

		netbuf_delete(buf);
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

	xTaskCreate(e131task, "e131task", 768, NULL, 8, NULL);
}

