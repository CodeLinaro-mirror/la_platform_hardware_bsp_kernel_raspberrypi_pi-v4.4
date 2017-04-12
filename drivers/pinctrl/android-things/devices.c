/*
 * devices.c
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

#include <linux/amba/bus.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>

#include "runtimepinconfig.h"
#include "platform_devices.h"

DEFINE_MUTEX(sysfs_mutex);
static struct pin_device **pin_array;

static inline struct pin_device *get_pin_device_by_pin(u32 pin)
{
	if (!pin_array || pin >= pin_count)
		return NULL;

	return pin_array[pin];
}

/*
 * Call a function for each pin in a device tree node. Look up the pin_device
 * represnting the pin and pass that to the function. Don't call the function if
 * no pin_device is found for a pin.
 */
static int device_for_each_pin(struct device_node *node,
			       int (*fn)(struct pin_device *pin))
{
	struct device_node *state;
	struct pin_device *pin_dev;
	int i = 0;
	u32 j;
	u32 pin;
	int ret = 0;

	while ((state = of_parse_phandle(node, STATE_0, i++))) {
		j = 0;

		while (!of_property_read_u32_index(state, PROP_PINS, j++,
		       &pin)) {
			if (!(pin_dev = get_pin_device_by_pin(pin)))
				/*
				 * This is normal for devices such as uart0,
				 * which has a third non-user pin in its device
				 * tree entry.
				 */
				continue;

			if ((ret = fn(pin_dev)))
				break;
		}

		of_node_put(state);

		if (ret)
			return ret;
	}

	return 0;
}

/*
 * track_pin_device() - Create a pin_device for a platform_device that has just
 * been registered, or update it if one already exists. This function should be
 * called from the driver's probe function.
 *
 * @dev:	The platform_device for this pin.
 * @class:	The class to use when creating sysfs files for this device. For
 *		example, using the pinctrl class causes files to be created in
 *		/sys/class/pinctrl.
 *
 * Returns the new (or updated) pin_device on success, NULL on failure.
 */
struct pin_device *track_pin_device(struct platform_device *dev,
				    struct class *class)
{
	const char *name;
	struct device *chardev;
	struct pin_device *pin_dev;
	int ret;
	u32 pin;

	if (!pin_array) {
		pr_err(TAG "pin device array uninitialized\n");
		return NULL;
	}

	if (of_property_read_string(dev->dev.of_node, "name", &name)) {
		dev_err(&dev->dev, "no name property found\n");
		return NULL;
	}

	if ((ret = get_pin(dev->dev.of_node, &pin))) {
		dev_err(&dev->dev, "no pin property found\n");
		return NULL;
	}

	if (pin >= pin_count) {
		dev_err(&dev->dev, "pin property %d out of range %d\n", pin,
			pin_count);
		return NULL;
	}

	/* This pin device may be returning after having been unregistered. */
	if ((pin_dev = get_pin_device_by_pin(pin))) {
		dev_dbg(&dev->dev, "tracking existing device\n");

		if (pin_dev->device != NULL)
			dev_warn(&dev->dev, "device is being re-added to pin device %d\n",
				 pin);

		pin_dev->set_gpio = 0;
		pin_dev->device = dev;

		return pin_dev;
	}

	if (!(pin_dev = kmalloc(sizeof(*pin_dev), GFP_KERNEL))) {
		pr_err(TAG "kmalloc failed %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	chardev = device_create(class, NULL, MKDEV(0, 0), pin_dev, "%s", name);
	if (IS_ERR(chardev)) {
		dev_err(&dev->dev, "unable to create char device");
		kfree(pin_dev);
		return NULL;
	}

	pin_dev->pin = pin;
	pin_dev->set_gpio = 0;
	pin_dev->device = dev;
	pin_dev->of_node = of_node_get(dev->dev.of_node);
	pin_dev->char_device = chardev;
	pin_array[pin] = pin_dev;

	dev_dbg(&dev->dev, "tracking new device\n");

	return pin_dev;
}

/*
 * The pin device has been unregistered, but we still want to keep most of its
 * data around in case it needs to be registered again. This function should be
 * called from the driver's remove function.
 */
void untrack_pin_device(struct pin_device *dev)
{
	dev->device = NULL;
}

/*
 * Unregister a generic device. Use the device's bus to determine its type, and
 * call the unregister function for that device type. Perform additional cleanup
 * for AMBA devices.
 */
static void unregister_device(struct device *dev)
{
	struct amba_device *adev;
	int ret;

	if (!dev) {
		return;
	} else if (dev->bus == &platform_bus_type) {
		platform_device_unregister(to_platform_device(dev));
	} else if (dev->bus == &amba_bustype) {
		adev = to_amba_device(dev);

		/*
		 * For whatever reason, the AMBA driver doesn't call
		 * release_resource (or amba_device_release doesn't get
		 * called). Call it here so we're able to add the device back
		 * later.
		 */
		if (adev->res.parent) {
			if ((ret = release_resource(&adev->res))) {
				dev_warn(dev, "release_resource failed with %d\n",
					 ret);
			}
		}

		amba_device_unregister(adev);
	} else {
		dev_warn(dev, "can't unregister with unknown bus type %s\n",
			 dev->bus->name);
	}
}

/* Register a device by its device tree node. */
static int register_device_by_node(struct device_node *node)
{
	struct device *parent;
	int ret;

	/*
	 * Mark the node as unpopulated so it will get registered by
	 * of_platform_popluate.
	 */
	of_node_clear_flag(node, OF_POPULATED);
	parent = find_device_by_node(node->parent);
	if ((ret = of_platform_populate(node->parent, NULL, NULL, parent))) {
		/*
		 * Something went wrong trying to bring the pin device back. Set
		 * this flag so we don't unintentionally attempt to bring it
		 * back next time we call of_platform_populate.
		 */
		of_node_set_flag(node, OF_POPULATED);
		pr_err(TAG "unable to register device for %s\n", node->name);
	} else {
		pr_debug(TAG "registered device for %s\n", node->name);
	}

	put_device(parent);

	return ret;
}

/*
 * If always_unreg_aux is set, we must unregister the auxiliary device any time
 * we unregister the primary device. Otherwise we can leave the auxiliary device
 * up while changing the pins.
 */
static void unregister_device_and_maybe_aux(struct device *dev,
					    struct bcm_device *bdev)
{
	struct device *aux_dev;

	if (bdev->always_unreg_aux && bdev->aux_dev.of_node) {
		aux_dev = find_device_by_node(bdev->aux_dev.of_node);
		unregister_device(aux_dev);
	}

	unregister_device(dev);
}

/*
 * If always_unreg_aux is set, we must unregister the auxiliary device BEFORE
 * unregistering the primary device. Otherwise unregister the primary device
 * first, then the auxiliary device.
 */
static void unregister_device_and_aux(struct device *dev,
				      struct bcm_device *bdev)
{
	struct device *aux_dev = NULL;

	if (bdev->aux_dev.of_node)
		aux_dev = find_device_by_node(bdev->aux_dev.of_node);

	if (bdev->always_unreg_aux) {
		unregister_device(aux_dev);
		unregister_device(dev);
	} else {
		unregister_device(dev);
		unregister_device(aux_dev);
	}
}

/* Register a device, and its auxiliary device of always_unreg_aux is set. */
static int register_device_and_maybe_aux(struct bcm_device *dev)
{
	int ret = register_device_by_node(dev->node.of_node);

	if (ret)
		return ret;

	if (dev->aux_dev.of_node && dev->always_unreg_aux)
		return register_device_by_node(dev->aux_dev.of_node);

	return 0;
}

/* Register a device and its auxiliary device. */
static int register_device_and_aux(struct bcm_device *dev)
{
	int ret;

	if (!dev->aux_dev.of_node)
		return register_device_by_node(dev->node.of_node);

	if (dev->always_unreg_aux) {
		if ((ret = register_device_by_node(dev->node.of_node)))
			return ret;
		return register_device_by_node(dev->aux_dev.of_node);
	}

	if ((ret = register_device_by_node(dev->aux_dev.of_node)))
		return ret;
	return register_device_by_node(dev->node.of_node);

	return 0;
}

/* Restore the device's default pin configuration and register it. */
static inline int register_default_device(struct bcm_device *dev)
{
	int ret = set_device_config(dev, NULL);

	if (ret)
		return ret;

	return register_device_and_aux(dev);
}

/*
 * Unregister all peripheral devices we know about and prepare their device tree
 * properties for our use.
 */
int unregister_platform_devices(void)
{
	struct device *dev;
	struct bcm_device *bdev;
	int ret = 0;

	if (pin_array) {
		pr_warn(TAG "pin array is already initialized\n");
	} else {
		pin_array = kcalloc(pin_count, sizeof(*pin_array),
				    GFP_KERNEL);
		if (!pin_array) {
			pr_err(TAG "kmalloc failed %s:%d\n", __FILE__,
			       __LINE__);
			return -ENOMEM;
		}
	}

	for (bdev = platform_devices; bdev->name != NULL; bdev++) {
		if (!bdev->node.path)
			continue;

		/* Populate the device and auxiliary device nodes. */
		bdev->node.of_node = of_find_node_by_path(bdev->node.path);
		if (!bdev->node.of_node) {
			pr_warn(TAG "unable to find %s in the device tree\n",
				bdev->node.path);
			continue;
		}

		if (bdev->aux_dev.path)
			bdev->aux_dev.of_node = of_find_node_by_path(
					bdev->aux_dev.path);

		/* Unregister the device and auxiliary device. */
		if (!bdev->use_default) {
			dev = find_device_by_node(bdev->node.of_node);
			unregister_device_and_aux(dev, bdev);
		}

		if ((ret = expand_property(bdev, PROP_PULL, 0)))
			return ret;
		if ((ret = expand_property(bdev, PROP_FUNC,
					   bdev->pin_groups[0].function)))
			return ret;

		if (!bdev->use_default)
			ret = set_device_config(bdev, &bdev->pin_groups[0]);
	}

	return ret;
}

/*
 * Find the device using this pin. The returned device may be a pin device or
 * peripheral device. We register pin devices for each pin when a peripheral
 * device gets unregistered, so returning NULL here should never happen. The
 * caller must call device_put on the returned device.
 */
static struct device *find_active_device_by_pin(u32 pin,
						struct bcm_device **dev)
{
	struct device *pdev;
	int has_pin;
	size_t i;

	for (i = 0; platform_devices[i].name != NULL; i++) {
		if (!platform_devices[i].node.path)
			continue;

		if (!platform_devices[i].node.of_node) {
			pr_warn(TAG "%s has no device tree node\n",
				platform_devices[i].name);
			continue;
		}

		pdev = find_device_by_node(platform_devices[i].node.of_node);
		has_pin = device_has_pin(platform_devices[i].node.of_node, pin);
		if (pdev && has_pin) {
			/*
			 * We are assuming only one registered device has this
			 * pin in its pins property.
			 */

			if (dev)
				*dev = &platform_devices[i];

			return pdev;
		}
	}

	return NULL;
}

static int register_pin_device(struct pin_device *dev)
{
	return (!dev->device) ? register_device_by_node(dev->of_node) : 0;
}

static int set_gpio_flag(struct pin_device *pin)
{
	pin->set_gpio = 1;
	return 0;
}

static int clear_gpio_flag(struct pin_device *pin)
{
	pin->set_gpio = 0;
	return 0;
}

/*
 * Free a pin for use by a device. If this pin is set to GPIO simply unregister
 * its pin device. If this pin is owned by another device, unregister that
 * device and set set_gpio for each of its pins. Later we will register a new
 * pin device for every pin with set_gpio set.
 */
static int unregister_pin_for_device(struct pin_device *pin)
{
	struct device *otherdev;
	struct bcm_device *dev;

	if (pin->device) {
		/* This pin is just owned by a pin device, so unregister it. */
		platform_device_unregister(pin->device);
	} else if ((otherdev = find_active_device_by_pin(pin->pin, &dev))) {
		/*
		 * This pin is owned by a platform device. For each pin owned by
		 * this device, set its set_gpio flag. Later we will clear the
		 * flags of those pins used by the platform device we are
		 * bringing up, and will create pin devices for those that
		 * remain. This prevents having to use a ton of nested loops.
		 */
		unregister_device_and_aux(otherdev, dev);

		device_for_each_pin(dev->node.of_node, set_gpio_flag);

		if (dev->use_default)
			return register_default_device(dev);
	} else {
		/*
		 * Nothing is using this pin. It was (hopefully) being used by
		 * the platform device we will be bringing back up.
		 */
	}

	return 0;
}

/*
 * Return a pointer to the pin property at the specified index. This pointer
 * can be used to read or modify the property. A device's pin list may be spread
 * across several nodes and properties.
 */
static __be32 *find_pin_property(struct device_node *node, const char *prop,
				 int index)
{
	struct property *pin_prop;
	struct device_node *state;
	int i = 0;
	int j = 0;
	int length;

	while ((state = of_parse_phandle(node, STATE_0, i++))) {
		pin_prop = of_find_property(state, prop, &length);
		of_node_put(state);

		if (!pin_prop) {
			pr_warn(TAG "no property \"%s\" for %s[%d]\n",
				prop, node->name, i);
			continue;
		}

		length /= sizeof(__be32);
		if (index >= j && index < j + length)
			return ((__be32 *)pin_prop->value) + index - j;

		j += length;
	}

	return NULL;
}

/*
 * Set this pin to GPIO. If a device owns this pin, unregister it and register
 * new pin devices for each of its pins.
 */
static inline int set_function_gpio(struct pin_device *dev)
{
	struct device *pdev;
	struct bcm_device *bdev;
	int ret;

	if (dev->device) {
		dev_dbg(&dev->device->dev, "already set to gpio\n");
		return 0;
	}

	if (!(pdev = find_active_device_by_pin(dev->pin, &bdev))) {
		/*
		 * Nothing is using this pin, so go ahead and register a pin
		 * device for it. We should never get here.
		 */
		pr_warn(TAG "no device is using pin %d\n", dev->pin);
		return register_pin_device(dev);
	}

	/* Unregister the platform device using this pin. */
	unregister_device_and_aux(pdev, bdev);

	dev_dbg(pdev, "unregistered to free pin %d\n", dev->pin);

	/*
	 * Register pin devices for pins previously used by the platform
	 * device.
	 */
	ret = device_for_each_pin(bdev->node.of_node, register_pin_device);
	if (ret)
		return ret;

	if (bdev->use_default)
		return register_default_device(bdev);

	return 0;
}

/* Replace a pin used by a device and register a pin device for the old pin. */
static inline int replace_pin(struct bcm_device *bcm_dev,
			      struct pin_group *group, u32 pin)
{
	struct pin_device *pin_dev;
	int ret;
	__be32 *pin_prop;
	__be32 *function;
	u32 pin_index;
	u32 old_pin;

	pin_index = pin - group->base;
	pin_prop = find_pin_property(bcm_dev->node.of_node, PROP_PINS,
				     pin_index);

	if (!pin_prop) {
		pr_err(TAG "unable to find pin index %d in %s\n",
		       pin_index, bcm_dev->name);
		return -EINVAL;
	}

	old_pin = be32_to_cpup(pin_prop);

	*pin_prop = cpu_to_be32p(&pin);

	if (!(pin_dev = get_pin_device_by_pin(old_pin))) {
		/*
		 * This happens when we are bringing up a device that
		 * was previously using non-user pins.
		 */
	} else if ((ret = register_pin_device(pin_dev))) {
		return ret;
	}

	/* Set the pin's function property in the device tree. */
	function = find_pin_property(bcm_dev->node.of_node, PROP_FUNC,
				     pin_index);
	if (!function) {
		pr_err(TAG "unable to find pin function index %d in %s\n",
		       pin_index, bcm_dev->name);
		return -EINVAL;
	}

	*function = cpu_to_be32p(&group->function);

	return 0;
}

/*
 * Set a pin's function while holding sysfs_lock. If the device for this
 * function is registered and doesn't own this pin, set the pin we replace to
 * GPIO. Unregister all devices owning pins that this device needs and set the
 * now unused pins to GPIO.
 */
static inline int __set_function(struct pin_device *dev,
				 struct bcm_device *bcm_dev,
				 struct  pin_group *group)
{
	struct device *new_dev;
	struct device *exdev;
	struct bcm_device *exbdev;
	struct pin_device *pin_dev;
	size_t i = 0;
	int has_pin;
	int ret = 0;

	if (!bcm_dev->node.path)
		return set_function_gpio(dev);

	if (!bcm_dev->node.of_node) {
		pr_err(TAG "%s has no device tree node\n", bcm_dev->name);
		return  -EINVAL;
	}

	new_dev = find_device_by_node(bcm_dev->node.of_node);
	has_pin = device_has_pin(bcm_dev->node.of_node, dev->pin);
	if (new_dev && has_pin) {
		/* The device already owns this pin and is registered. */
		dev_dbg(new_dev, "pin %d is already reserved\n", dev->pin);
		put_device(new_dev);
		return 0;
	}

	/* Unregister this device so we can change its pins. */
	unregister_device_and_maybe_aux(new_dev, bcm_dev);

	if (bcm_dev->use_default) {
		/*
		 * By default, this device uses non-user pins. Rather than mix
		 * user and non-user pins, set this device to use all user pins
		 * as soon as it is requested.
		 */
		if ((ret = set_device_config(bcm_dev, group))) {
			pr_err(TAG "unable to set config for %s\n",
			       bcm_dev->name);
			return ret;
		}
	} else if (!has_pin) {
		if ((ret = replace_pin(bcm_dev, group, dev->pin)))
			return ret;
	}

	/*
	 * Unregister devices using pins we need, and set the set_gpio flag for
	 * each newly freed pin.
	 */
	if ((ret = device_for_each_pin(bcm_dev->node.of_node,
				       unregister_pin_for_device))) {
		pr_err(TAG "unable to unregister pin devices needed by %s\n",
		       bcm_dev->name);
		return ret;
	}

	/* Unregister each mutually exclusive device. */
	for (i = 0; bcm_dev->excl && (exbdev = bcm_dev->excl[i]); i++) {
		if ((exdev = find_device_by_node(exbdev->node.of_node))) {
			device_for_each_pin(exbdev->node.of_node,
					    set_gpio_flag);

			unregister_device_and_aux(exdev, exbdev);

			if (exbdev->use_default)
				if ((ret = register_default_device(exbdev)))
					return ret;
		}
	}

	/* Clear the set_gpio flag for every pin we're using. */
	device_for_each_pin(bcm_dev->node.of_node, clear_gpio_flag);

	/* Register a pin device for each pin we're not using. */
	for (i = 0, ret = 0; i < pin_count; i++) {
		if (!(pin_dev = get_pin_device_by_pin(i)))
			continue;

		if (pin_dev->set_gpio)
			if ((ret = register_pin_device(pin_dev)))
				return ret;

		pin_dev->set_gpio = 0;
	}

	/*
	 * Register this platform device. If the device was not previously
	 * registered, bring the auxiliary device up as well.
	 */
	if (new_dev)
		return register_device_and_maybe_aux(bcm_dev);
	else
		return register_device_and_aux(bcm_dev);
}

int set_function(struct pin_device *dev, struct bcm_device *bcm_dev,
		 struct  pin_group *group)
{
	int ret;

	mutex_lock(&sysfs_mutex);
	ret = __set_function(dev, bcm_dev, group);
	mutex_unlock(&sysfs_mutex);

	return ret;
}


/*
 * Find a matching pin group and return this pin's index in the property
 * list.
 */
static inline u32 get_pin_property_index(struct bcm_device *dev, u32 pin)
{
	struct pin_group *groups;
	size_t i;

	groups = dev->pin_groups;
	for (i = 0 ; dev->pin_group_count; i++) {
		if (pin_in_group(pin, groups[i].base, dev->pin_count))
			return pin - groups[i].base;
	}

	return -1;
}

/* Set a GPIO pin's pull-up/pull-down resistor configuration. */
static inline int set_resistor_gpio(struct pin_device *dev, u32 resistor)
{
	__be32 *pull;

	if (!(pull = find_pin_property(dev->of_node, PROP_PULL, 0))) {
		pr_err(TAG "unable to find resistor index 0 in pin %d\n",
		       dev->pin);
		return -EINVAL;
	} else if (*pull == cpu_to_be32p(&resistor)) {
		pr_debug(TAG "pin %d is already set to resistor %d\n",
			 dev->pin, resistor);
		return 0;
	}

	/* Disable the device before changing the resistor. */
	platform_device_unregister(dev->device);

	*pull = cpu_to_be32p(&resistor);

	return register_pin_device(dev);
}

/*
 * Set a pin's pull-up/pull-down resistor configuration while holding
 * sysfs_lock.
 */
static inline int __set_resistor(struct pin_device *dev, u32 resistor)
{
	struct device *pdev;
	struct bcm_device *bdev;
	int index;
	__be32 *pull;

	if (dev->device)
		return set_resistor_gpio(dev, resistor);

	if ((pdev = find_active_device_by_pin(dev->pin, &bdev))) {
		index = get_pin_property_index(bdev, dev->pin);
		if (index == -1) {
			dev_err(pdev, "could not get resistor index for pin %d\n",
				dev->pin);
			return -EINVAL;
		}
	} else {
		pr_err(TAG "no device is using pin %d\n", dev->pin);
		put_device(pdev);
		return -ENODEV;
	}

	pull = find_pin_property(bdev->node.of_node, PROP_PULL, index);
	if (!pull) {
		dev_err(pdev, "unable to find pin index %d\n", index);
		put_device(pdev);
		return -EINVAL;
	} else if (*pull == cpu_to_be32p(&resistor)) {
		dev_dbg(pdev, "pin %d is already set to resistor %d\n",
			dev->pin, resistor);
		put_device(pdev);
		return 0;
	}

	/* Disable the device before changing the resistor. */
	unregister_device_and_maybe_aux(pdev, bdev);

	*pull = cpu_to_be32p(&resistor);

	return register_device_and_maybe_aux(bdev);
}

int set_resistor(struct pin_device *dev, u32 resistor)
{
	int ret;

	mutex_lock(&sysfs_mutex);
	ret = __set_resistor(dev, resistor);
	mutex_unlock(&sysfs_mutex);

	return ret;
}
