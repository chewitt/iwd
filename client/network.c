/*
 *
 *  Wireless daemon for Linux
 *
 *  Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ell/ell.h>

#include "dbus-proxy.h"
#include "display.h"
#include "network.h"

struct network {
	bool connected;
	char *identity;
	char *name;
	char *type;
	const struct proxy_interface *device;
};

static void check_errors_method_callback(struct l_dbus_message *message,
								void *user_data)
{
	dbus_message_has_error(message);
}

bool network_is_connected(const char *path)
{
	const struct network *network;
	const struct proxy_interface *proxy =
			proxy_interface_find(IWD_NETWORK_INTERFACE, path);

	network = proxy_interface_get_data(proxy);

	return network->connected;
}

void network_connect(const char *path)
{
	const struct proxy_interface *proxy =
			proxy_interface_find(IWD_NETWORK_INTERFACE, path);

	if (!proxy)
		return;

	proxy_interface_method_call(proxy, "Connect", "",
						check_errors_method_callback);
}

static void set_name(void *data, struct l_dbus_message_iter *variant)
{
	struct network *network = data;
	const char *value;

	l_free(network->name);

	if (!l_dbus_message_iter_get_variant(variant, "s", &value)) {
		network->name = NULL;

		return;
	}

	network->name = l_strdup(value);
}

static void set_connected(void *data, struct l_dbus_message_iter *variant)
{
	struct network *network = data;
	bool value;

	if (!l_dbus_message_iter_get_variant(variant, "b", &value)) {
		network->connected = false;

		return;
	}

	network->connected = value;
}

static void set_device(void *data, struct l_dbus_message_iter *variant)
{
	struct network *network = data;
	const char *path;

	if (!l_dbus_message_iter_get_variant(variant, "o", &path)) {
		network->device = NULL;

		return;
	}

	network->device = proxy_interface_find(IWD_DEVICE_INTERFACE, path);
}

static void set_type(void *data, struct l_dbus_message_iter *variant)
{
	struct network *network = data;
	const char *value;

	l_free(network->type);

	if (!l_dbus_message_iter_get_variant(variant, "s", &value)) {
		network->type = NULL;

		return;
	}

	network->type = l_strdup(value);
}

static const struct proxy_interface_property network_properties[] = {
	{ "Name",       "s", set_name},
	{ "Connected",  "b", set_connected},
	{ "Device",     "o", set_device},
	{ "Type",       "s", set_type},
	{ }
};

static const char *network_identity(void *data)
{
	struct network *network = data;

	if (!network->identity)
		network->identity =
			l_strdup_printf("%s %s", network->name, network->type);

	return network->identity;
}

static void network_display_inline(const char *margin, const void *data)
{
	const struct network *network = data;

	display("%s%s %s %s\n", margin, network->name ? network->name : "",
			network->type ? network->type : "",
			network->connected ? "connected" : "diconnected");
}

static void *network_create(void)
{
	return l_new(struct network, 1);
}

static void network_destroy(void *data)
{
	struct network *network = data;

	l_free(network->name);
	l_free(network->type);
	l_free(network->identity);

	network->device = NULL;

	l_free(network);
}

static const struct proxy_interface_type_ops ops = {
	.create = network_create,
	.destroy = network_destroy,
	.display = network_display_inline,
	.identity = network_identity,
};

static struct proxy_interface_type network_interface_type = {
	.interface = IWD_NETWORK_INTERFACE,
	.properties = network_properties,
	.ops = &ops,
};

static int network_interface_init(void)
{
	proxy_interface_type_register(&network_interface_type);

	return 0;
}

static void network_interface_exit(void)
{
	proxy_interface_type_register(&network_interface_type);
}

INTERFACE_TYPE(network_interface_type, network_interface_init,
						network_interface_exit)
