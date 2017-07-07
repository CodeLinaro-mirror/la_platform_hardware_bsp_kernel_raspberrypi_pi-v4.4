/*
 * platform_devices.c
 *
 * Runtime pin configuration for Raspberry Pi
 *
 * Copyright (C) 2017 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "runtimepinconfig.h"
#include "platform_devices.h"

/*
 * platform_devices - Specifies devices we want to control pinmuxing for.
 *
 * The device tree tells us which pins are used by which devices by default, but
 * it doesn't tell us which other pins can be used by those devices. It also
 * doesn't tell us which devices are mutually exclusive, or which devices these
 * devices depend on.
 */

const u32 pin_min = 2;
const u32 pin_max = 27;

const char *pin_prefix = "BCM";
const char *pin_path_prefix = "/soc/android-things-pins/BCM";

struct bcm_device platform_devices[] = {
	{
		.name = "SPI0",
		.node = { .path = "/soc/spi@7e204000" },
		.aux_dev = { .path = NULL },
		.use_default = 0,
		.always_unreg_aux = 0,
		.init_unreg = 0,
		.pin_count = 5,
		.pin_pull = (u32 []) { NONE, NONE, NONE, NONE, NONE },
		.pin_group_count = 1,
		.pin_groups = (struct pin_group []) {
			{
				.base = 7,
				.function = ALT0
			}
		},
		.excl = NULL
	}, {
		.name = "PWM",
		.node = { .path = "/soc/pwm@7e20c000" },
		.aux_dev = { .path = "/soc/cprman@7e101000" },
		.use_default = 0,
		.always_unreg_aux = 0,
		.init_unreg = 1,
		.pin_count = 2,
		.pin_pull = (u32 []) { NONE, NONE, },
		.pin_group_count = 2,
		.pin_groups = (struct pin_group []) {
			{
				.base = 12,
				.function = ALT0
			}, {
				.base = 18,
				.function = ALT5
			}
		},
		.excl = (struct bcm_device *[]) {
			&platform_devices[5],
			NULL
		}
	}, {
		.name = "I2C1",
		.node = { .path = "/soc/i2c@7e804000" },
		.aux_dev = { .path = NULL },
		.use_default = 0,
		.always_unreg_aux = 0,
		.init_unreg = 1,
		.pin_count = 2,
		.pin_pull = (u32 []) { NONE, NONE },
		.pin_group_count = 1,
		.pin_groups = (struct pin_group []) {
			{
				.base = 2,
				.function = ALT0
			}
		},
		.excl = NULL
	}, {
		.name = "UART0",
		.node = { .path = "/soc/uart@7e201000" },
		.aux_dev = { .path = NULL },
		.use_default = 1,
		.always_unreg_aux = 0,
		.init_unreg = 0,
		.pin_count = 2,
		.pin_pull = (u32 []) { NONE, UP },
		.pin_group_count = 2,
		.pin_groups = (struct pin_group []) {
			{
				.base = 32,
				.function = ALT3
			}, {
				.base = 14,
				.function = ALT0
			}
		},
		.excl = NULL
	}, {
		.name = "UART1",
		.node = { .path = "/soc/uart@7e215040" },
		.aux_dev = { .path = NULL },
		.use_default = 0,
		.always_unreg_aux = 0,
		.init_unreg = 0,
		.pin_count = 2,
		.pin_pull = (u32 []) { NONE, UP },
		.pin_group_count = 1,
		.pin_groups = (struct pin_group []) {
			{
				.base = 14,
				.function = ALT5
			}
		},
		.excl = NULL
	}, {
		.name = "I2S1",
		.node = { .path = "/soc/i2s@7e203000" },
		.aux_dev = { .path = "/soc/sound" },
		.use_default = 0,
		.always_unreg_aux = 1,
		.init_unreg = 1,
		.pin_count = 4,
		.pin_pull = (u32 []) { NONE, NONE, NONE, NONE },
		.pin_group_count = 1,
		.pin_groups = (struct pin_group []) {
			{
				.base = 18,
				.function = ALT0
			}
		},
		.excl = (struct bcm_device *[]) {
			&platform_devices[1],
			NULL
		}
	}, {
		.name = "GPIO",
		.node = { .path = NULL },
		.aux_dev = { .path = NULL },
		.use_default = 0,
		.always_unreg_aux = 0,
		.init_unreg = 0,
		.pin_count = 26,
		.pin_group_count = 1,
		.pin_groups = (struct pin_group []) {
			{
				.base = 2,
				.function = GPIO
			}
		},
		.excl = NULL
	}, {
		.name = NULL
	}
};

struct bcm_resistor platform_resistors[] = {
	{
		.name = "NONE",
		.resistor = NONE
	}, {
		.name = "DOWN",
		.resistor = DOWN
	}, {
		.name = "UP",
		.resistor = UP
	}, {
		.name = NULL
	}
};
