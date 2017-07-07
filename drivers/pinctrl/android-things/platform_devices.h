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
	GPIO_IN = 0,
	GPIO_OUT = 1,
	ALT0 = 4,
	ALT1 = 5,
	ALT2 = 6,
	ALT3 = 7,
	ALT4 = 3,
	ALT5 = 2
};

/*
 * struct pin_function - Defines the pin number and hardware-specific function
 * number for a pin used by a device. We assume that devices may have different
 * groups of pins that they can use and that mixing between groups is allowed,
 * but no one pin can take on multiple functions for one device. We also assume
 * that each group of pins for a device follows the same order of functions,
 * and that this order is followed by pinctrl properties in the device tree.
 * The index member here is used to specify the pin's place in its group.
 */
struct pin_function {
	u32 pin;
	int index;
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
 * @use_default:	Whether or not we should register this device on a set
 *			of default pin_functions if nobody is using it. For
 *			example, uart0 on the Raspberry Pi 3 is also used For
 *			Bluetooth, so we need to register uart0 again when the
 *			user stops using it directly. bcm_device.pins[n][0] are
 *			the default pin_functions used.
 * @always_unreg_aux:	Whether or not we should unregister the auxiliary device
 *			whenever we unregister this device. This flag also
 *			controls the order in which the devices get
 *			registered/unregistered.
 * @init_unreg:	Whether or not we should unregister the device upon loading the
 *		module. This is useful for mutually exclusive devices which all
 *		may be registered at boot.
 * @pin_count:	The number of pins used by this peripheral.
 * @pin_groups:	The number of pin groups that can be used by this device.
 * @pin_pull:	Specifies default resistor values for this device. Only used
 *		with use_default.
 * @pins:	An array of arrays of pin_functions available to this device.
 *		The top-level array has bcm_device.pin_count entries, and each
 *		array in it has bcm_device.pin_groups entries. The first
 *		pin_functions in each array make up the default group for each
 *		device.
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
	int init_unreg:1;
	int pin_count;
	int pin_groups;
	u32 *pin_pull;
	struct pin_function **pins;
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
extern const u32 pin_min;
extern const u32 pin_max;
extern const char *pin_prefix;
extern const char *pin_path_prefix;

#endif /* PLATFORM_DEVICES_H_ */
