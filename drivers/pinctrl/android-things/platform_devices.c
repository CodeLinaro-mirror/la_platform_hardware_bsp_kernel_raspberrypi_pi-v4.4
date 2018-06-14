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
		.init_unreg = 1,
		.pin_count = 5,
		.pin_pull = (u32 []) { NONE, NONE, NONE, NONE, NONE },
		.pin_groups = 1,
		.pins = (struct pin_function *[]) {
			(struct pin_function []) {
				{ .pin = 7, .index = 0, .function = GPIO_OUT }
			}, (struct pin_function []) {
				{ .pin = 8, .index = 1, .function = GPIO_OUT }
			}, (struct pin_function []) {
				{ .pin = 9, .index = 2, .function = ALT0 }
			}, (struct pin_function []) {
				{ .pin = 10, .index = 3, .function = ALT0 }
			}, (struct pin_function []) {
				{ .pin = 11, .index = 4, .function = ALT0 }
			}
		},
		.excl = NULL
	}, {
		.name = "SPI1",
		.node = { .path = "/soc/spi@7e215080" },
		.aux_dev = { .path = NULL },
		.use_default = 0,
		.always_unreg_aux = 0,
		.init_unreg = 1,
		.pin_count = 6,
		.pin_pull = (u32 []) { NONE, NONE, NONE, NONE, NONE, NONE },
		.pin_groups = 1,
		.pins = (struct pin_function *[]) {
			(struct pin_function []) {
				{ .pin = 16, .index = 0, .function = GPIO_OUT }
			}, (struct pin_function []) {
				{ .pin = 17, .index = 1, .function = GPIO_OUT }
			}, (struct pin_function []) {
				{ .pin = 18, .index = 2, .function = GPIO_OUT }
			}, (struct pin_function []) {
				{ .pin = 19, .index = 3, .function = ALT4 }
			}, (struct pin_function []) {
				{ .pin = 20, .index = 4, .function = ALT4 }
			}, (struct pin_function []) {
				{ .pin = 21, .index = 5, .function = ALT4 }
			}
		},
		.excl = NULL
	}, {
		.name = "PWM",
		.node = { .path = "/soc/pwm@7e20c000" },
		.aux_dev = { .path = NULL },
		.use_default = 0,
		.always_unreg_aux = 0,
		.init_unreg = 1,
		.pin_count = 2,
		.pin_pull = (u32 []) { NONE, NONE, },
		.pin_groups = 2,
		.pins = (struct pin_function *[]) {
			(struct pin_function []) {
				{ .pin = 18, .index = 0, .function = ALT5 },
				{ .pin = 12, .index = 0, .function = ALT0 }
			}, (struct pin_function []) {
				{ .pin = 13, .index = 1, .function = ALT0 },
				{ .pin = 19, .index = 1, .function = ALT5 }
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
		.pin_groups = 1,
		.pins = (struct pin_function *[]) {
			(struct pin_function []) {
				{ .pin = 2, .index = 0, .function = ALT0 }
			}, (struct pin_function []) {
				{ .pin = 3, .index = 1, .function = ALT0 }
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
		.pin_groups = 2,
		.pins = (struct pin_function *[]) {
			(struct pin_function []) {
				{ .pin = 32, .index = 0, .function = ALT3 },
				{ .pin = 14, .index = 0, .function = ALT0 }
			}, (struct pin_function []) {
				{ .pin = 33, .index = 1, .function = ALT3 },
				{ .pin = 15, .index = 1, .function = ALT0 }
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
		.pin_groups = 1,
		.pins = (struct pin_function *[]) {
			(struct pin_function []) {
				{ .pin = 14, .index = 0, .function = ALT5 }
			}, (struct pin_function []) {
				{ .pin = 15, .index = 1, .function = ALT5 }
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
		.pin_groups = 1,
		.pins = (struct pin_function *[]) {
			(struct pin_function []) {
				{ .pin = 18, .index = 0, .function = ALT0 }
			}, (struct pin_function []) {
				{ .pin = 19, .index = 1, .function = ALT0 }
			}, (struct pin_function []) {
				{ .pin = 20, .index = 2, .function = ALT0 }
			}, (struct pin_function []) {
				{ .pin = 21, .index = 3, .function = ALT0 }
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
		.pin_count = 0,
		.pin_groups = 0,
		.pins = NULL,
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
