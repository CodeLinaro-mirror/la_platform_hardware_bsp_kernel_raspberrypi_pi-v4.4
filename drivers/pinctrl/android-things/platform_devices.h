/*
 * platform_devices.h
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

#ifndef PLATFORM_DEVICES_H_
#define PLATFORM_DEVICES_H_

#define PROP_PINS "brcm,pins"
#define PROP_FUNC "brcm,function"
#define PROP_PULL "brcm,pull"

#define STATE_0 "pinctrl-0"

/*
 * struct node_path - Matches a device tree path to the corresponding node
 * structure. Rather than look up the path every time we need to find a node,
 * save a pointer to the node here.
 */
struct node_path {
	const char *path;
	struct device_node *of_node;
};

/* Broadcom pin function numbers. See drivers/pinctrl/bcm/pinctrl-bcm2835.c */
enum bcm_fsel {
	GPIO = 0,
	ALT0 = 4,
	ALT1 = 5,
	ALT2 = 6,
	ALT3 = 7,
	ALT4 = 3,
	ALT5 = 2
};

/*
 * struct pin_group - Defines the starting pin and hardware-specific function
 * number for a group of pins used by a peripheral. We assume that each group
 * has the same function number and contains bcm_device.pin_count consecutive
 * pins.
 */
struct pin_group {
	u32 base;
	enum bcm_fsel function;
};

/*
 * struct bcm_device - Contains details about relevant peripherals on the chip.
 *
 * @name:	A string used to look up this device. Used to assign a pin to a
 *		device through the sysfs interface.
 * @node:	A node_path containing the device tree node for this device.
 * @aux_dev:	A node_path containing the device tree node for this device's
 *		auxiliary device. The auxiliary device may depend on this
 *		device, or this device may depend on it.
 * @use_default:	Whether or not we should register this device on
 *			a default pin_group if nobody is using it. For example,
 *			uart0 on the Raspberry Pi 3 is also used for Bluetooth,
 *			so we need to register it again when the user stops
 *			using it directly. pin_groups[0] is the default
 *			pin_group.
 * @always_unreg_aux:	Whether or not we should unregister the auxiliary device
 *			whenever we unregister this device. This flag also
 *			controls the order in which the devices get
 *			registered/unregistered.
 * @pin_count:	The number of pins used by this peripheral.
 * @pin_pull:	Specifies default resistor values for this device. Only used
 *		with use_default.
 * @pin_group_count:	The number of pin_groups available to this device and
 *			the length of the following array.
 * @pin_groups:	Array of pin_groups for this device.
 * @excl:	NULL-terminated array of bcm_devices that are mutually exclusive
 *		with this device. Whenever we register this device, we must
 *		first unregister every device in this array. For example, i2s
 *		and pwm are mutually exclusive despite the fact that they can
 *		use non-overlapping groups of pins.
 */
struct bcm_device {
	const char *name;
	struct node_path node;
	struct node_path aux_dev;
	int use_default:1;
	int always_unreg_aux:1;
	int pin_count;
	u32 *pin_pull;
	int pin_group_count;
	struct pin_group *pin_groups;
	struct bcm_device **excl;
};

/* Broadcom pin resistor numbers. */
enum bcm_rsel {
	NONE = 0,
	DOWN = 1,
	UP = 2
};

/*
 * struct bcm_resistor - Matches a string description to a hardware-specific
 * resistor number. Used to set the resistor through the sysfs interface.
 */
struct bcm_resistor {
	const char *name;
	enum bcm_rsel resistor;
};

extern struct bcm_device platform_devices[];
extern struct bcm_resistor platform_resistors[];
extern const int pin_count;

#endif /* PLATFORM_DEVICES_H_ */
