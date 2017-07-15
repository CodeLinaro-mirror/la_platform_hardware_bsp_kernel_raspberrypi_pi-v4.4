/*
 * runtimepinconfig.h
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

#ifndef RUNTIMEPINCONFIG_H_
#define RUNTIMEPINCONFIG_H_

#include <linux/device.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>

#define TAG "runtimepinconfig: "

struct pin_function;
struct bcm_device;

/*
 * struct pin_device - Holds information about a pin device that has been
 * registered with our driver.
 *
 * @pin:	The physical pin number for this device.
 * @set_gpio:	Set by the driver to determine which pin devices to register
 *		unregister.
 * @device:	The platform_device that represents this pin. We set this to
 *		NULL when the pin device gets unregistered.
 * @of_node:	The device tree node for this device. We save a pointer to it
 *		here in case the pin device gets unregistered.
 * @char_device:	The character device used to create the sysfs files for
 *			this pin.
 */
struct pin_device {
	u32 pin;
	int set_gpio:1;
	struct platform_device *device;
	struct device_node *of_node;
	struct device *char_device;
};

ssize_t function_store(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t buflen);
ssize_t resistor_store(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t buflen);

int set_function(struct pin_device *dev, struct bcm_device *bcm_dev,
		 struct pin_function *pin);
int set_resistor(struct pin_device *dev, u32 resistor);

struct pin_device *track_pin_device(struct platform_device *dev);
void untrack_pin_device(struct pin_device *dev);

int platform_devices_init(struct class *class);
int pin_devices_init(void);

int get_pin(struct device_node *node, u32 *pin);
int device_has_pin(struct device_node *node, u32 pin);
int expand_property(struct bcm_device *dev, const char *prop_name,
		    bool require);
struct device *find_device_by_node(struct device_node *node);
int set_device_default_config(struct bcm_device *dev);

#endif /* RUNTIMEPINCONFIG_H_ */
