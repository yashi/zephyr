/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <device.h>
#include <drivers/led.h>
#include <drivers/can.h>

#define LED0_NODE DT_ALIAS(led0)

#if 0
#define LED0	DEVICE_DT_NAME(DT_INST(0, sc_led))
#else
#define LED0	""
#endif

#if 0
#define CAN0	DEVICE_DT_NAME(DT_INST(0, sc_can))
#else
#define CAN0	""
#endif


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

const struct device *led;

static void read_test(void)
{
	for (int i=0; i<30; i++) {
		uint32_t irq = sys_read32(CAN_IRQSTATUS);
		printk("IRQ %08x\n", irq);
		if (irq & 0x20) {
			printk("MSG1   %08x\n", sys_read32(CAN_RXMSG1) >> 1);
			sys_write32(0x20, CAN_IRQSTATUS);
		/* } else { */
		/* 	printk("."); */
		}

		led_on(led, i);
		led_off(led, i-1);
		k_sleep(K_SECONDS(1));
	}
}

	const struct zcan_frame life = {
		.id = 0xf0,
		.rtr = CAN_DATAFRAME,
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.dlc = 8,
		.data_32 = {
			sys_cpu_to_be32(0x12345678),
			sys_cpu_to_be32(0xdeadbeef),
		},
		/* .data = { */
		/* 	0x12, 0x34, 0x56, 0x78, */
		/* 	0xde, 0xad, 0xbe, 0xef, */
		/* }, */
	};

	const struct zcan_frame life2 = {
		.id = 0xf0,
		.rtr = CAN_DATAFRAME,
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.dlc = 8,
		.data_32 = {
			0x12345678,
			0xdeadbeef,
		},
		/* .data = { */
		/* 	0x12, 0x34, 0x56, 0x78, */
		/* 	0xde, 0xad, 0xbe, 0xef, */
		/* }, */
	};

	const struct zcan_frame life3 = {
		.id = 0xf0,
		.rtr = CAN_DATAFRAME,
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.dlc = 8,
		.data = {
			0x12, 0x34, 0x56, 0x78,
			0xde, 0xad, 0xbe, 0xef,
		},
	};

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

	led = device_get_binding(LED0);
	printk("led device %p\n", led);

	const struct device *can;
	can = device_get_binding(CAN0);
	printk("can device %p\n", can);

	if (can) {
		can_send(can, &life, K_FOREVER, NULL, NULL);
		can_send(can, &life2, K_FOREVER, NULL, NULL);
		can_send(can, &life3, K_FOREVER, NULL, NULL);
	}

	printk("done\n");
}
