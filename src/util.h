/*
 *
 *  Wireless daemon for Linux
 *
 *  Copyright (C) 2013-2014  Intel Corporation. All rights reserved.
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

#ifndef __UTIL_H
#define __UTIL_H

#include <stdint.h>
#include <unistd.h>

#define align_len(len, boundary) (((len)+(boundary)-1) & ~((boundary)-1))

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

const char *util_ssid_to_utf8(size_t len, const uint8_t *ssid);
bool util_ssid_is_utf8(size_t len, const uint8_t *ssid);
const char *util_address_to_string(const uint8_t *addr);
bool util_string_to_address(const char *str, uint8_t *addr);

static inline uint8_t util_bit_field(const uint8_t oct, int start, int num)
{
	unsigned char mask = (1 << num) - 1;
	return (oct >> start) & mask;
}

static inline bool util_is_bit_set(const uint8_t oct, int bit)
{
	int mask = 1 << bit;
	return oct & mask ? true : false;
}

static inline bool util_mem_is_zero(const uint8_t *field, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++)
		if (field[i] != 0)
			return false;

	return true;
}

#endif /* __UTIL_H */
