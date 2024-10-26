/*
 * Copyright (c) 2024, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>

#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/transport/smp_reassembly.h>

#include "error_messages.h"

#define SMP_LORAWAN_TRANSPORT SMP_USER_DEFINED_TRANSPORT

extern int hfclk_enable(void);
extern int hfclk_disable(void);

LOG_MODULE_REGISTER(smp_lorawan, CONFIG_MCUMGR_TRANSPORT_LORAWAN_LOG_LEVEL);

static void smp_lorawan_downlink(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
			      uint8_t len, const uint8_t *hex_data);

static int smp_lorawan_uplink(struct net_buf *nb);

static uint16_t smp_lorawan_get_mtu(const struct net_buf *nb);

static struct lorawan_downlink_cb lorawan_smp_downlink_cb = {
	.port = CONFIG_MCUMGR_TRANSPORT_LORAWAN_PORT,
	.cb = smp_lorawan_downlink
};

struct smp_transport smp_lorawan_transport = {
	.functions.output = smp_lorawan_uplink,
	.functions.get_mtu = smp_lorawan_get_mtu,
};

#ifdef CONFIG_SMP_CLIENT
struct smp_client_transport_entry smp_lorawan_client_transport = {
	.smpt = &smp_lorawan_transport;
	.smpt_type = SMP_LORAWAN_TRANSPORT;
};
#endif

struct smp_lorawan_uplink_message_t {
	void *fifo_reserved;
	struct net_buf *nb;
	struct k_sem my_sem;
};

#ifdef CONFIG_MCUMGR_TRANSPORT_LORAWAN_POLL_FOR_DATA
static struct k_thread smp_lorawan_thread;
K_KERNEL_STACK_MEMBER(smp_lorawan_stack, 1650);
K_FIFO_DEFINE(smp_lorawan_fifo);

static struct smp_lorawan_uplink_message_t empty_message = {
	.nb = NULL,
};

static void smp_lorawan_uplink_thread(void *p1, void *p2, void *p3)
{
	struct smp_lorawan_uplink_message_t *msg;

	while (1) {
		msg = k_fifo_get(&smp_lorawan_fifo, K_FOREVER);
		uint16_t size = 0;
		uint16_t pos = 0;

		if (msg->nb != NULL) {
			size = msg->nb->len;
		}

		(void)hfclk_enable();

		while (pos < size || size == 0) {
			uint8_t *data = NULL;
			uint8_t data_size;
			uint8_t temp;
			uint8_t tries = 3;

			lorawan_get_payload_sizes(&data_size, &temp);

			if (data_size > size) {
				data_size = size;
			}

			if (size > 0) {
				data = net_buf_pull_mem(msg->nb, data_size);
			}

			while (tries > 0) {
				int rc;

				rc = lorawan_send(CONFIG_MCUMGR_TRANSPORT_LORAWAN_PORT, data, data_size,
						  (CONFIG_MCUMGR_TRANSPORT_LORAWAN_CONFIRMED_PACKETS ?
						   LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED));

				if (rc != 0) {
					--tries;
				} else {
					break;
				}
			}

			if (size == 0) {
				break;
			}

			pos += data_size;
		}

		/* For empty packets, do not trigger semaphore */
		if (size != 0) {
		    k_sem_give(&msg->my_sem);
		}

		(void)hfclk_disable();
	}
}
#endif

static void smp_lorawan_downlink(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
			      uint8_t len, const uint8_t *hex_data)
{
	ARG_UNUSED(data_pending);
	ARG_UNUSED(rssi);
	ARG_UNUSED(snr);

	if (port == CONFIG_MCUMGR_TRANSPORT_LORAWAN_PORT) {
#ifdef CONFIG_MCUMGR_TRANSPORT_LORAWAN_REASSEMBLY
		int rc;

		if (len == 0) {
			/* Empty packet is used to clear partially queued data */
			(void)smp_reassembly_drop(&smp_lorawan_transport);
LOG_ERR("empty message, clear queue");
		} else {
LOG_ERR("smp message %d bytes", len);
			rc = smp_reassembly_collect(&smp_lorawan_transport, hex_data, len);

			if (rc == 0) {
				rc = smp_reassembly_complete(&smp_lorawan_transport, false);

				if (rc) {
					LOG_ERR("LoRaWAN SMP reassembly complete failed: %d", rc);
				}
else {
LOG_ERR("smp reassembly finished");
}
			} else if (rc < 0) {
				LOG_ERR("LoRaWAN SMP reassembly collect failed: %d", rc);
			} else {
				LOG_ERR("LoRaWAN SMP expected data left: %d", rc);

#ifdef CONFIG_MCUMGR_TRANSPORT_LORAWAN_POLL_FOR_DATA
				/* Send empty LoRaWAN packet to receive next packet from server */
			        k_fifo_put(&smp_lorawan_fifo, &empty_message);
#else
				/* Queue empty message */
				error_message_lock();
				error_message_add_error(NULL, 0);
				error_message_unlock();
#endif
			}
		}
#else
		if (len > sizeof(struct smp_hdr)) {
			struct net_buf *nb;

			nb = smp_packet_alloc();

			if (!nb) {
				LOG_ERR("LoRaWAN SMP packet allocation failure");
				return;
			}

			net_buf_add_mem(nb, hex_data, len);
			smp_rx_req(&smp_lorawan_transport, nb);
		} else {
			LOG_ERR("Invalid LoRaWAN SMP downlink");
		}
#endif
	} else {
		LOG_ERR("Invalid LoRaWAN SMP downlink");
	}
}

static int smp_lorawan_uplink(struct net_buf *nb)
{
	int rc = 0;

	(void)hfclk_enable();

#ifdef CONFIG_MCUMGR_TRANSPORT_LORAWAN_FRAGMENTED_UPLINKS
	uint16_t pos = 0;

	while (pos < nb->len) {
		uint16_t size = 0;

		while (pos < size || size == 0) {
			uint8_t *data = NULL;
			uint8_t data_size;
			uint8_t temp;
			uint8_t tries = 3;

			lorawan_get_payload_sizes(&data_size, &temp);

			if (data_size > size) {
				data_size = size;
			}

			if (size > 0) {
				data = net_buf_pull_mem(nb, data_size);
			}

			while (tries > 0) {
				int rc;

				rc = lorawan_send(CONFIG_MCUMGR_TRANSPORT_LORAWAN_PORT, data, data_size,
						  (CONFIG_MCUMGR_TRANSPORT_LORAWAN_CONFIRMED_PACKETS ?
						   LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED));

				if (rc != 0) {
					--tries;
				} else {
					break;
				}
			}

			if (size == 0) {
				break;
			}

			pos += data_size;
		}
	}
#else
	uint8_t data_size;
	uint8_t temp;

	lorawan_get_payload_sizes(&data_size, &temp);

	if (nb->len > data_size) {
		LOG_ERR("Cannot send LoRaWAN SMP message, too large. Message: %d, maximum: %d",
			nb->len, data_size);
	} else {
		rc = lorawan_send(CONFIG_MCUMGR_TRANSPORT_LORAWAN_PORT, nb->data, nb->len,
				  (CONFIG_MCUMGR_TRANSPORT_LORAWAN_CONFIRMED_PACKETS ?
				   LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED));

		if (rc != 0) {
			LOG_ERR("Failed to send LoRaWAN SMP message: %d", rc);
		}
	}
#endif

	(void)hfclk_disable();
	smp_packet_free(nb);

	return rc;
}

static uint16_t smp_lorawan_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);

	uint8_t max_data_size;
	uint8_t temp;

	lorawan_get_payload_sizes(&max_data_size, &temp);

	return (uint16_t)max_data_size;
}

static void smp_lorawan_start(void)
{
	int rc;

	rc = smp_transport_init(&smp_lorawan_transport);

#ifdef CONFIG_SMP_CLIENT
	if (rc == 0) {
		smp_client_transport_register(&smp_lorawan_client_transport);
	}
#endif

	if (rc == 0) {
		lorawan_register_downlink_callback(&lorawan_smp_downlink_cb);
	} else {
		LOG_ERR("Failed to init LoRaWAN MCUmgr SMP transport: %d", rc);
	}

#ifdef CONFIG_MCUMGR_TRANSPORT_LORAWAN_REASSEMBLY
	smp_reassembly_init(&smp_lorawan_transport);
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_LORAWAN_POLL_FOR_DATA
	k_thread_create(&smp_lorawan_thread, smp_lorawan_stack,
			K_KERNEL_STACK_SIZEOF(smp_lorawan_stack),
			smp_lorawan_uplink_thread, NULL, NULL, NULL,
			3, 0, K_FOREVER);

	k_thread_start(&smp_lorawan_thread);
#endif
}

MCUMGR_HANDLER_DEFINE(smp_lorawan, smp_lorawan_start);
