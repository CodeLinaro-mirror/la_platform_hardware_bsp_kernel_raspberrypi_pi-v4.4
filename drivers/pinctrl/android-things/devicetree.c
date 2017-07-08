/*
 * devicetree.c
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
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>

#include "runtimepinconfig.h"
#include "platform_devices.h"

/*
 * get_pin() - Get the first pin in a device tree node. Only intended to be
 * called for pin devices.
 *
 * @node:	The device tree node to search.
 * @pin:	Set to the pin number if it is found.
 *
 * Returns 0 if the pin number is found.
 */
int get_pin(struct device_node *node, u32 *pin)
{
	struct device_node *statenode;
	int ret;

	if (!(statenode = of_parse_phandle(node, STATE_0, 0)))
		return -EINVAL;

	ret = of_property_read_u32(statenode, PROP_PINS, pin);
	of_node_put(statenode);
	return ret;
}

/*
 * device_has_pin() - Check if a device tree node contains the specified pin.
 *
 * @node:	The device tree node to search.
 * @pin:	The pin number to look for.
 *
 * Returns 1 if the device contains the pin, 0 otherwise.
 */
int device_has_pin(struct device_node *node, u32 pin)
{
	struct device_node *pinstate;
	int i = 0;
	u32 prop_pin;
	u32 j = 0;

	while ((pinstate = of_parse_phandle(node, STATE_0, i++))) {
		j = 0;

		while (!of_property_read_u32_index(pinstate, PROP_PINS, j++,
						   &prop_pin)) {
			if (prop_pin == pin)
				return 1;
		}

		of_node_put(pinstate);
	}

	return 0;
}

/*
 * Creates a struct property with name prop and the specified length in
 * bytes.
 */
static inline struct property *create_property(const char *prop, int length)
{
	struct property *ret;

	if (!(ret = kzalloc(sizeof(*ret), GFP_KERNEL)))
		goto err_alloc_struct;
	if (!(ret->name = kstrdup(prop, GFP_KERNEL)))
		goto err_alloc_name;
	if (!(ret->value = kzalloc(length, GFP_KERNEL)))
		goto err_alloc_value;

	ret->length = length;
	of_property_set_flag(ret, OF_DYNAMIC);
	return ret;

err_alloc_value:
	kfree(ret->name);
err_alloc_name:
	kfree(ret);
err_alloc_struct:
	return NULL;
}

static inline void fill_value(__be32 *array, u32 value, int length)
{
	int size = length / sizeof(*array);
	int i;

	for (i = 0; i < size; i++)
		*array++ = cpu_to_be32p(&value);
}

/*
 * expand_property() - Checks a device's pinctrl properties, and expands them to
 * the proper length if necessary. For example, the resistor property may
 * specify one value for all pins, one value for each pin, or no value. We want
 * to have one value for each pin so we can configure pins individually, no
 * matter which device is using them.
 *
 * @dev:	The device to check.
 * @prop_name:	The string name of the property to check.
 * @value:	The new value to fill an expanded property with.
 *
 * Returns 0 on success.
 */
int expand_property(struct bcm_device *dev, const char *prop_name, u32 value)
{
	struct of_changeset changeset;
	struct device_node *prop;
	struct property *pins, *pull;
	struct property *new_pull;
	int pins_length, pull_length;
	int total_pins = 0;
	int ret;
	unsigned long action;
	int i = 0;

	of_changeset_init(&changeset);

	while ((prop = of_parse_phandle(dev->node.of_node, STATE_0, i++))) {
		pins = of_find_property(prop, PROP_PINS, &pins_length);
		pull = of_find_property(prop, prop_name, &pull_length);

		if (!pins) {
			pr_warn(TAG "%s[%d] has no %s property\n", dev->name,
				i - 1, PROP_PINS);
			of_node_put(prop);
			continue;
		}

		total_pins += (pins_length / sizeof(value));
		if (total_pins > dev->pin_count) {
			pr_warn(TAG "%s stopping at %d pins out of %d\n",
				dev->name, dev->pin_count, total_pins);
			of_node_put(prop);
			break;
		}

		if (pull && pull_length >= pins_length) {
			/*
			 * The property already exists and is of the correct
			 * length.
			 */
			of_node_put(prop);
			continue;
		}

		new_pull = create_property(prop_name, pins_length);
		if (!new_pull)
			goto err_prop;

		fill_value(new_pull->value, value, pins_length);

		action = (!pull) ? OF_RECONFIG_ADD_PROPERTY
				: OF_RECONFIG_UPDATE_PROPERTY;

		if (of_changeset_action(&changeset, action, prop, new_pull)) {
			pr_err(TAG "unable to update %s property for %s[%d]\n",
			       prop_name, dev->name, i - 1);
			goto err_apply;
		} else {
			pr_debug(TAG "updated %s property for %s[%d]\n",
				 prop_name, dev->name, i - 1);
		}

		of_node_put(prop);
	}

	if ((ret = of_changeset_apply(&changeset)))
		pr_err(TAG "unable to apply changeset for %s\n", dev->name);

	of_changeset_destroy(&changeset);

	return ret;

err_apply:
	kfree(new_pull);
err_prop:
	of_changeset_destroy(&changeset);
	of_node_put(prop);
	return -ENOMEM;
}

static int node_match_device(struct device *dev, void *data)
{
	return (dev->of_node == data);
}

/*
 * find_device_by_node() - Finds a device from it's device tree node. The caller
 * must call put_device on the returned device.
 *
 * Returns the device or NULL if no such device is registered.
 */
struct device *find_device_by_node(struct device_node *node)
{
	struct platform_device *pdev;

	if ((pdev = of_find_device_by_node(node)))
		return &pdev->dev;
	else
		return bus_find_device(&amba_bustype, NULL, node,
				       node_match_device);
}

/*
 * set_device_config() - Sets a device's device tree configuration.
 *
 * @dev:	The device to change.
 * @group:	The new pin group to set, or NULL for the default.
 *
 * Returns 0 on success.
 */
int set_device_config(struct bcm_device *dev, struct pin_group *group)
{
	struct device_node *prop;
	struct property *pins, *function, *pull;
	__be32 *list;
	__be32 *flist = NULL;
	__be32 *plist = NULL;
	u32 pin;
	int length;
	int i, j, total;

	if (!group)
		group = &dev->pin_groups[0];

	i = 0;
	j = 0;
	while ((prop = of_parse_phandle(dev->node.of_node, STATE_0, i++))) {
		pins = of_find_property(prop, PROP_PINS, &length);
		function = of_find_property(prop, PROP_FUNC, NULL);
		pull = of_find_property(prop, PROP_PULL, NULL);

		if (!pins || !function || !pull) {
			pr_warn(TAG "%s[%d] is missing a property\n",
				dev->name, i - 1);
			of_node_put(prop);
			continue;
		}

		if (function->length != length || pull->length != length) {
			pr_err(TAG "%s[%d] property size mismatch\n",
			       dev->name, i - 1);
			goto err_prop;
		}

		list = pins->value;
		flist = function->value;
		plist = pull->value;

		length /= sizeof(*list);
		total = j + length;

		if (total > dev->pin_count) {
			pr_warn(TAG "%s has %d pins, more than the %d we know about\n",
				dev->name, total, dev->pin_count);
		}

		for ( ; j < total && j < dev->pin_count; j++) {
			pin = group->base + j;
			*list++ = cpu_to_be32p(&pin);
			*flist++ = cpu_to_be32p(&group->function);
			*plist++ = cpu_to_be32p(&dev->pin_pull[j]);
		}

		of_node_put(prop);

		if (total > dev->pin_count) {
			pr_warn(TAG "%s has %d pins, more than the %d we know about\n",
				dev->name, total, dev->pin_count);
			break;
		}

		j = total;
	}

	return 0;

err_prop:
	of_node_put(prop);
	return -EINVAL;
}
