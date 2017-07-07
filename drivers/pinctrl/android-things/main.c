/*
 * main.c
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

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "runtimepinconfig.h"

MODULE_LICENSE("GPL v2");

static struct platform_driver pin_driver;

DEVICE_ATTR_WO(function);
DEVICE_ATTR_WO(resistor);

static struct attribute *pinctrl_attrs[] = {
	&dev_attr_function.attr,
	&dev_attr_resistor.attr,
	NULL
};
ATTRIBUTE_GROUPS(pinctrl);

static struct class pinctrl_class = {
	.name = "pinctrl",
	.owner = THIS_MODULE,
	.dev_groups = pinctrl_groups
};

static int pin_probe(struct platform_device *dev)
{
	struct pin_device *pin_dev;

	if ((pin_dev = track_pin_device(dev, &pinctrl_class)))
		dev_set_drvdata(&dev->dev, pin_dev);

	return 0;
}

static int pin_remove(struct platform_device *dev)
{
	untrack_pin_device(dev_get_drvdata(&dev->dev));
	return 0;
}

static int __init runtimepinconfig_init(void)
{
	if (class_register(&pinctrl_class)) {
		pr_err(TAG "unable create pinctrl class\n");
		goto err_class;
	}

	if (unregister_platform_devices()) {
		pr_err(TAG "unable to unregister platform devices\n");
		goto err_unregister_or_driver;
	}

	if (__platform_driver_register(&pin_driver, THIS_MODULE)) {
		pr_err(TAG "unable to register pin driver\n");
		goto err_unregister_or_driver;
	}

	pr_debug(TAG "module loaded\n");
	return 0;

err_unregister_or_driver:
	class_unregister(&pinctrl_class);
err_class:
	return -ECANCELED;
}

static const struct of_device_id pin_match[] = {
	{ .compatible = "google,android-things-pins" },
	{ }
};
MODULE_DEVICE_TABLE(of, pin_match);

static struct platform_driver pin_driver = {
	.probe = pin_probe,
	.remove = pin_remove,
	.driver = {
		.name = "android-things-pins",
		.of_match_table = pin_match
	}
};

module_init(runtimepinconfig_init);
