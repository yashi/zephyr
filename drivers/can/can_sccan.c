/*
 * Copyright (c) 2021 Space Cubics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sc_can

#include <kernel.h>
#include <errno.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <drivers/can.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(can_driver, CONFIG_CAN_LOG_LEVEL);

#include "can_utils.h"

#ifdef CONFIG_CAN_FD_MODE
#error SC CAN driver does not support CAN FD
#endif

#define SC_IPVER_MAJOR(v) (((v) & 0xff000000) >> 24)
#define SC_IPVER_MINOR(v) (((v) & 0x00ff0000) >> 16)
#define SC_IPVER_PATCH(v) (((v) & 0x0000ffff) >>  0)

/* Registers */
#define CAN_BASEADDR (0x40600000)
#define CAN_ENABLE      (CAN_BASEADDR | 0x00)
#define CAN_TIMEQUANTUM (CAN_BASEADDR | 0x08)
#define CAN_BITTIMING   (CAN_BASEADDR | 0x0c)
#define CAN_ERRCOUNT    (CAN_BASEADDR | 0x10)
#define CAN_STATUS      (CAN_BASEADDR | 0x18)
#define CAN_IRQSTATUS   (CAN_BASEADDR | 0x20)
#define CAN_IRQENABLE   (CAN_BASEADDR | 0x24)
#define CAN_TX_MSG1     (CAN_BASEADDR | 0x30)
#define CAN_TX_MSG2     (CAN_BASEADDR | 0x34)
#define CAN_TX_MSG3     (CAN_BASEADDR | 0x38)
#define CAN_TX_MSG4     (CAN_BASEADDR | 0x3c)
#define CAN_RXMSG1      (CAN_BASEADDR | 0x50)
#define CAN_IPVERSION   (CAN_BASEADDR | 0xf000)

#define CAN_TXID1(x)    (x << 21)
#define CAN_TXSRTR(x)   (x << 20)
#define CAN_TXIDE(x)    (x << 19)
#define CAN_TXID_EX1(x) ((x & GENMASK(28,18)) << 3)
#define CAN_TXID_EX2(x) ((x & GENMASK(17,0)) << 1)
#define CAN_TXERTR(x)   (x)

static int sc_can_set_mode(const struct device *dev, enum can_mode mode)
{
	return 0;
}

static int sc_can_set_timing(const struct device *dev,
			     const struct can_timing *timing,
			     const struct can_timing *timing_data)
{
	return 0;
}

static inline uint32_t sc_can_msg1(uint32_t id, bool extended, bool rtr)
{
	uint32_t ret;

	if (extended) {
		ret = (CAN_TXID_EX1(id) | CAN_TXSRTR(1) | CAN_TXIDE(extended) |
		       CAN_TXID_EX2(id) | CAN_TXERTR(rtr));
	} else {
		ret = (CAN_TXID1(id) | CAN_TXSRTR(rtr) | CAN_TXIDE(extended));
	}

	return ret;
}

static int sc_can_send(const struct device *dev, const struct zcan_frame *msg,
		       k_timeout_t timeout, can_tx_callback_t callback,
		       void *callback_arg)
{
	size_t length;
	uint32_t msg1;

	uint32_t status;
	uint32_t irq;

	//printk("status %08x\n", sys_read32(CAN_STATUS));
	length = can_dlc_to_bytes(msg->dlc);

	LOG_DBG("Sending %d bytes on %s. Id: 0x%x, ID type: %s %s",
		length, dev->name, msg->id,
		msg->id_type == CAN_STANDARD_IDENTIFIER ? "standard" : "extended",
		msg->rtr == CAN_DATAFRAME ? "data" : "remote");

	msg1 = sc_can_msg1(msg->id, msg->id_type, msg->rtr);
	//printk("msg1 %08x\n", msg1);
	sys_write32(msg1, CAN_TX_MSG1);
	sys_write32(length, CAN_TX_MSG2);
	sys_write32(sys_be32_to_cpu(msg->data_32[0]), CAN_TX_MSG3);
	sys_write32(sys_be32_to_cpu(msg->data_32[1]), CAN_TX_MSG4);

	irq = sys_read32(CAN_IRQSTATUS);
	status = sys_read32(CAN_STATUS);
	//printk("irq %08x\n", irq);
	//printk("status %08x\n", status);
	irq = sys_read32(CAN_IRQSTATUS);
	//printk("irq %08x\n", irq);

	return CAN_TX_OK;
}

static int sc_can_attach_isr(const struct device *dev,
			     can_rx_callback_t isr, void *cb_arg,
			     const struct zcan_filter *filter)
{
	return 0;
}

static void sc_can_detach(const struct device *dev, int filter_nr)
{
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int sc_can_recover(const struct device *dev, k_timeout_t timeout)
{
	return 0;
}
#endif

static enum can_state sc_can_get_state(const struct device *dev,
				       struct can_bus_err_cnt *err_cnt)
{
	return CAN_ERROR_ACTIVE;
}

static void sc_can_register_state_change_isr(const struct device *dev,
					     can_state_change_isr_t isr)
{
}

static int sc_can_get_core_clock(const struct device *dev, uint32_t *rate)
{
	return 0;
}

static void sc_can_isr(const struct device *dev)
{
	printk("IRQ Status %08x\n", sys_read32(CAN_IRQSTATUS));
	sys_write32(0xffffffff, CAN_IRQSTATUS);
}

static void dump_reg(void)
{
	printk("Enable %08x\n", sys_read32(CAN_ENABLE));
	printk("TQP    %08x\n", sys_read32(CAN_TIMEQUANTUM));
	printk("BIT    %08x\n", sys_read32(CAN_BITTIMING));
	printk("IRQ EN %08x\n", sys_read32(CAN_IRQENABLE));
}

static int sc_can_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t v;

	dump_reg();

	sys_write32(1, CAN_TIMEQUANTUM);
	sys_write32(0x1a7, CAN_BITTIMING);
	sys_write32(1, CAN_ENABLE);
	sys_write32(1, CAN_IRQENABLE);

	dump_reg();

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    sc_can_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	v = sys_read32(CAN_IPVERSION);
	printk("Space Cubics CAN controller v%d.%d.%d initialized\n",
	       SC_IPVER_MAJOR(v), SC_IPVER_MINOR(v), SC_IPVER_PATCH(v));

	return 0;
}

static const struct can_driver_api sc_can_api = {
	.set_mode = sc_can_set_mode,
	.set_timing = sc_can_set_timing,
	.send = sc_can_send,
	.attach_isr = sc_can_attach_isr,
	.detach = sc_can_detach,
	.get_state = sc_can_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = sc_can_recover,
#endif
	.register_state_change_isr = sc_can_register_state_change_isr,
	.get_core_clock = sc_can_get_core_clock,
	.timing_min = {
		.sjw = 0x1,
		.prop_seg = 0x00,
		.phase_seg1 = 0x04,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x4,
		.prop_seg = 0x00,
		.phase_seg1 = 0x10,
		.phase_seg2 = 0x08,
		.prescaler = 0x400
	}
};

DEVICE_DT_INST_DEFINE(0, &sc_can_init, NULL, NULL, NULL,
		      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,
		      &sc_can_api);
