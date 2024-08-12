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
		} else {
			rc = smp_reassembly_collect(&smp_lora_transport, hex_data, len);

			if (rc == 0) {
				rc = smp_reassembly_complete(&smp_lora_transport, false);

				if (rc) {
					LOG_ERR("LoRa SMP reassembly complete failed: %d", rc);
				}
			} else if (rc < 0) {
				LOG_ERR("LoRa SMP reassembly collect failed: %d", rc);
			} else {
				LOG_DBG("LoRa SMP expected data left: %d", rc);

#ifdef CONFIG_MCUMGR_TRANSPORT_LORA_POLL_FOR_DATA
				/* Send empty LoRa packet to receive next packet from server */
				rc = lorawan_send(CONFIG_MCUMGR_TRANSPORT_LORA_PORT, NULL, 0,
					  (CONFIG_MCUMGR_TRANSPORT_LORA_CONFIRMED_PACKETS ?
					   LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED));

				if (rc) {
					LOG_ERR("LoRa SMP send empty message failed: %d", rc);
				}
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
	int rc;
	uint8_t data_size;
	uint8_t temp;

#ifdef CONFIG_MCUMGR_TRANSPORT_LORA_FRAGMENTED_UPLINKS
	uint16_t pos = 0;

	while (pos < nb->len) {
		uint8_t *data;

		lorawan_get_payload_sizes(&data_size, &temp);

		if (data_size > (nb->len - pos)) {
			data_size = (nb->len - pos);
		}

		data = net_buf_pull_mem(nb, data_size);

		rc = lorawan_send(CONFIG_MCUMGR_TRANSPORT_LORA_PORT, data, data_size,
				  (CONFIG_MCUMGR_TRANSPORT_LORA_CONFIRMED_PACKETS ?
				   LORAWAN_MSG_CONFIRMED : LORAWAN_MSG_UNCONFIRMED));

		if (rc != 0) {
			LOG_ERR("Failed to send LoRa SMP message: %d, sent %d of %d", rc, pos,
				nb->len);
			break;
		}

		pos += data_size;
	}
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
}

MCUMGR_HANDLER_DEFINE(smp_lora, smp_lora_start);
