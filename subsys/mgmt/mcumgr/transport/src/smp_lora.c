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

#define SMP_LORA_TRANSPORT SMP_USER_DEFINED_TRANSPORT

LOG_MODULE_REGISTER(smp_lora, CONFIG_MCUMGR_TRANSPORT_LORA_LOG_LEVEL);

static void smp_lora_downlink(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
			      uint8_t len, const uint8_t *hex_data);

static int smp_lora_uplink(struct net_buf *nb);

static uint16_t smp_lora_get_mtu(const struct net_buf *nb);

static struct lorawan_downlink_cb lora_smp_downlink_cb = {
	.port = CONFIG_MCUMGR_TRANSPORT_LORA_PORT,
	.cb = smp_lora_downlink
};

struct smp_transport smp_lora_transport = {
	.functions.output = smp_lora_uplink,
	.functions.get_mtu = smp_lora_get_mtu,
};

#ifdef CONFIG_SMP_CLIENT
struct smp_client_transport_entry smp_lora_client_transport = {
	.smpt = &smp_lora_transport;
	.smpt_type = SMP_LORA_TRANSPORT;
};
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_LORA_POLL_FOR_DATA
static struct k_thread smp_lora_thread;
K_KERNEL_STACK_MEMBER(smp_lora_stack, 1650);
K_FIFO_DEFINE(smp_lora_fifo);

struct smp_lora_uplink_message_t {
	void *fifo_reserved;
	struct net_buf *nb;
	struct k_sem my_sem;
};

static struct smp_lora_uplink_message_t empty_message = {
	.nb = NULL,
};

static void smp_lora_uplink_thread(void *p1, void *p2, void *p3)
{
	struct smp_lora_uplink_message_t *msg;

	while (1) {
		msg = k_fifo_get(&smp_lora_fifo, K_FOREVER);
		uint16_t size = 0;
		uint16_t pos = 0;

		if (msg->nb != NULL) {
			size = msg->nb->len;
		}

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

				rc = lorawan_send(CONFIG_MCUMGR_TRANSPORT_LORA_PORT, data, data_size,
						  (CONFIG_MCUMGR_TRANSPORT_LORA_CONFIRMED_PACKETS ?
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
	}
}
#endif

static void smp_lora_downlink(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
			      uint8_t len, const uint8_t *hex_data)
{
	ARG_UNUSED(data_pending);
	ARG_UNUSED(rssi);
	ARG_UNUSED(snr);

	if (port == CONFIG_MCUMGR_TRANSPORT_LORA_PORT) {
#ifdef CONFIG_MCUMGR_TRANSPORT_LORA_REASSEMBLY
		int rc;

		if (len == 0) {
			/* Empty packet is used to clear partially queued data */
			(void)smp_reassembly_drop(&smp_lora_transport);
LOG_ERR("empty message, clear queue");
		} else {
LOG_ERR("smp message %d bytes", len);
			rc = smp_reassembly_collect(&smp_lora_transport, hex_data, len);

			if (rc == 0) {
				rc = smp_reassembly_complete(&smp_lora_transport, false);

				if (rc) {
					LOG_ERR("LoRa SMP reassembly complete failed: %d", rc);
				}
else {
LOG_ERR("smp reassembly finished");
}
			} else if (rc < 0) {
				LOG_ERR("LoRa SMP reassembly collect failed: %d", rc);
			} else {
				LOG_ERR("LoRa SMP expected data left: %d", rc);

#ifdef CONFIG_MCUMGR_TRANSPORT_LORA_POLL_FOR_DATA
				/* Send empty LoRa packet to receive next packet from server */
			        k_fifo_put(&smp_lora_fifo, &empty_message);
#endif
			}
		}
#else
		if (len > sizeof(struct smp_hdr)) {
			struct net_buf *nb;

			nb = smp_packet_alloc();

			if (!nb) {
				LOG_ERR("LoRa SMP packet allocation failure");
				return;
			}

			net_buf_add_mem(nb, hex_data, len);
			smp_rx_req(&smp_lora_transport, nb);
		} else {
			LOG_ERR("Invalid LoRa SMP downlink");
		}
#endif
	} else {
		LOG_ERR("Invalid LoRa SMP downlink");
	}
}

static int smp_lora_uplink(struct net_buf *nb)
{
	int rc = 0;
	uint8_t data_size;
	uint8_t temp;

#ifdef CONFIG_MCUMGR_TRANSPORT_LORA_FRAGMENTED_UPLINKS
	struct smp_lora_uplink_message_t tx_data = {
		.nb = nb,
	};

	k_sem_init(&tx_data.my_sem, 0, 1);
        k_fifo_put(&smp_lora_fifo, &tx_data);
	k_sem_take(&tx_data.my_sem, K_FOREVER);
#else
	lorawan_get_payload_sizes(&data_size, &temp);

	if (nb->len > data_size) {
		LOG_ERR("Cannot send LoRa SMP message, too large. Message: %d, maximum: %d",
			nb->len, data_size);
	} else {
		rc = lorawan_send(CONFIG_MCUMGR_TRANSPORT_LORA_PORT, nb->data, nb->len,
				  (CONFIG_MCUMGR_TRANSPORT_LORA_CONFIRMED_PACKETS ?
				   LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED));

		if (rc != 0) {
			LOG_ERR("Failed to send LoRa SMP message: %d", rc);
		}
	}
#endif

	smp_packet_free(nb);

	return rc;
}

static uint16_t smp_lora_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);

	uint8_t max_data_size;
	uint8_t temp;

	lorawan_get_payload_sizes(&max_data_size, &temp);

	return (uint16_t)max_data_size;
}

static void smp_lora_start(void)
{
	int rc;

	rc = smp_transport_init(&smp_lora_transport);

#ifdef CONFIG_SMP_CLIENT
	if (rc == 0) {
		smp_client_transport_register(&smp_lora_client_transport);
	}
#endif

	if (rc == 0) {
		lorawan_register_downlink_callback(&lora_smp_downlink_cb);
	} else {
		LOG_ERR("Failed to init LoRa MCUmgr SMP transport: %d", rc);
	}

#ifdef CONFIG_MCUMGR_TRANSPORT_LORA_REASSEMBLY
	smp_reassembly_init(&smp_lora_transport);
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_LORA_POLL_FOR_DATA
	k_thread_create(&smp_lora_thread, smp_lora_stack,
			K_KERNEL_STACK_SIZEOF(smp_lora_stack),
			smp_lora_uplink_thread, NULL, NULL, NULL,
			3, 0, K_FOREVER);

	k_thread_start(&smp_lora_thread);
#endif
}

MCUMGR_HANDLER_DEFINE(smp_lora, smp_lora_start);
