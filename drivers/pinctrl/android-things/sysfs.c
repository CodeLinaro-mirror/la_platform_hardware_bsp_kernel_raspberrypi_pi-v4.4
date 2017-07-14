/*
 * sysfs.c
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

#include <linux/device.h>
#include <linux/of.h>

#include "runtimepinconfig.h"
#include "platform_devices.h"

/*
 * These functions are the sysfs interface. Parse the user input and match it to
 * a pin on a peripheral device or a resistor value.
 */
ssize_t function_store(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t bufsize)
{
	struct pin_device *pin_dev = dev_get_drvdata(dev);
	struct bcm_device *bcm_dev = NULL;
	size_t i, j, namelen, inlen;
	int ret;

	for (inlen = 0; buf[inlen] != '\n' && inlen < bufsize; inlen++)
		;

	/* Match the input buffer to a platform device name. */
	for (i = 0; platform_devices[i].name != NULL; i++) {
		namelen = strlen(platform_devices[i].name);
		if (inlen != namelen)
			continue;

		if (!strncmp(platform_devices[i].name, buf, namelen)) {
			bcm_dev = &platform_devices[i];
			break;
		}
	}

	if (!bcm_dev) {
		pr_warn(TAG "no matching platform device found on pin %d\n",
			pin_dev->pin);
		return -ENODEV;
	} else if (bcm_dev->pin_count == 0) {
		ret = set_function(pin_dev, bcm_dev, NULL);
		return (ret) ? ret : bufsize;
	}

	/* Loop over the possible pins for this device. */
	for (i = 0; i < bcm_dev->pin_count; i++) {
		for (j = 0; j < bcm_dev->pin_groups; j++) {
			if (bcm_dev->pins[i][j].pin == pin_dev->pin) {
				ret = set_function(pin_dev, bcm_dev,
						   &bcm_dev->pins[i][j]);
				return (ret) ? ret : bufsize;
			}
		}
	}

	pr_warn(TAG "no matching pin group for function %s on pin %d\n",
		bcm_dev->name, pin_dev->pin);
	return -EINVAL;
}

ssize_t resistor_store(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t bufsize)
{
	struct pin_device *pin_dev = dev_get_drvdata(dev);
	int ret;
	size_t i, namelen, inlen;

	for (inlen = 0; buf[inlen] != '\n' && inlen < bufsize; inlen++)
		;

	for (i = 0; platform_resistors[i].name; i++) {
		namelen = strlen(platform_resistors[i].name);
		if (inlen != namelen)
			continue;

		if (!strncmp(platform_resistors[i].name, buf, namelen)) {
			ret = set_resistor(pin_dev,
					   platform_resistors[i].resistor);
			return (ret) ? ret : bufsize;
		}
	}

	pr_warn(TAG "no matching resistor configuration found for pin %d\n",
		pin_dev->pin);
	return -EINVAL;
}
