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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ell/ell.h>

#include "src/sha1.h"
#include "src/eapol.h"
#include "src/crypto.h"

struct eapol_key_data {
	const unsigned char *frame;
	size_t frame_len;
	enum eapol_protocol_version protocol_version;
	uint16_t packet_len;
	enum eapol_descriptor_type descriptor_type;
	enum eapol_key_descriptor_version key_descriptor_version;
	bool key_type:1;
	bool install:1;
	bool key_ack:1;
	bool key_mic:1;
	bool secure:1;
	bool error:1;
	bool request:1;
	bool encrypted_key_data:1;
	bool smk_message:1;
	uint16_t key_length;
	uint64_t key_replay_counter;
	uint8_t key_nonce[32];
	uint8_t eapol_key_iv[16];
	uint8_t key_rsc[8];
	uint8_t key_mic_data[16];
	uint16_t key_data_len;
};

/* Random WPA EAPoL frame, using 2001 protocol */
static const unsigned char eapol_key_data_1[] = {
	0x01, 0x03, 0x00, 0x5f, 0xfe, 0x00, 0x89, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0xd5, 0xe2, 0x13, 0x9b, 0x1b, 0x1c, 0x1e,
	0xcb, 0xf4, 0xc7, 0x9d, 0xb3, 0x70, 0xcd, 0x1c, 0xea, 0x07, 0xf1, 0x61,
	0x76, 0xed, 0xa6, 0x78, 0x8a, 0xc6, 0x8c, 0x2c, 0xf4, 0xd7, 0x6f, 0x2b,
	0xf7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};

static struct eapol_key_data eapol_key_test_1 = {
	.frame = eapol_key_data_1,
	.frame_len = sizeof(eapol_key_data_1),
	.protocol_version = EAPOL_PROTOCOL_VERSION_2001,
	.packet_len = 95,
	.descriptor_type = EAPOL_DESCRIPTOR_TYPE_WPA,
	.key_descriptor_version = EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_MD5_ARC4,
	.key_type = true,
	.install = false,
	.key_ack = true,
	.key_mic = false,
	.secure = false,
	.error = false,
	.request = false,
	.encrypted_key_data = false,
	.smk_message = false,
	.key_length = 32,
	.key_replay_counter = 1,
	.key_nonce = { 0xd5, 0xe2, 0x13, 0x9b, 0x1b, 0x1c, 0x1e, 0xcb, 0xf4,
			0xc7, 0x9d, 0xb3, 0x70, 0xcd, 0x1c, 0xea, 0x07, 0xf1,
			0x61, 0x76, 0xed, 0xa6, 0x78, 0x8a, 0xc6, 0x8c, 0x2c,
			0xf4, 0xd7, 0x6f, 0x2b, 0xf7 },
	.eapol_key_iv = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_rsc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_mic_data = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_data_len = 0,
};

/* Random WPA2 EAPoL frame, using 2004 protocol */
static const unsigned char eapol_key_data_2[] = {
	0x02, 0x03, 0x00, 0x75, 0x02, 0x00, 0x8a, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x6a, 0xce, 0x64, 0xc1, 0xa6, 0x44,
	0xd2, 0x7b, 0x84, 0xe0, 0x39, 0x26, 0x3b, 0x63, 0x3b, 0xc3, 0x74, 0xe3,
	0x29, 0x9d, 0x7d, 0x45, 0xe1, 0xc4, 0x25, 0x44, 0x05, 0x48, 0x05, 0xbf,
	0xe5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x16, 0xdd, 0x14, 0x00, 0x0f, 0xac, 0x04, 0x05, 0xb1, 0xb6,
	0x8b, 0x5a, 0x91, 0xfc, 0x04, 0x06, 0x83, 0x84, 0x06, 0xe8, 0xd1, 0x5f,
	0xdb,
};

static struct eapol_key_data eapol_key_test_2 = {
	.frame = eapol_key_data_2,
	.frame_len = sizeof(eapol_key_data_2),
	.protocol_version = EAPOL_PROTOCOL_VERSION_2004,
	.packet_len = 117,
	.descriptor_type = EAPOL_DESCRIPTOR_TYPE_80211,
	.key_descriptor_version = EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_SHA1_AES,
	.key_type = true,
	.install = false,
	.key_ack = true,
	.key_mic = false,
	.secure = false,
	.error = false,
	.request = false,
	.encrypted_key_data = false,
	.smk_message = false,
	.key_length = 16,
	.key_replay_counter = 0,
	.key_nonce = { 0x12, 0x6a, 0xce, 0x64, 0xc1, 0xa6, 0x44, 0xd2, 0x7b,
			0x84, 0xe0, 0x39, 0x26, 0x3b, 0x63, 0x3b, 0xc3, 0x74,
			0xe3, 0x29, 0x9d, 0x7d, 0x45, 0xe1, 0xc4, 0x25, 0x44,
			0x05, 0x48, 0x05, 0xbf, 0xe5 },
	.eapol_key_iv = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_rsc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_mic_data = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_data_len = 22,
};

/* WPA2 frame, 1 of 4.  For parameters see eapol_4way_test */
static const unsigned char eapol_key_data_3[] = {
	0x02, 0x03, 0x00, 0x5f, 0x02, 0x00, 0x8a, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xc2, 0xbb, 0x57, 0xab, 0x58, 0x8f, 0x92,
	0xeb, 0xbd, 0x44, 0xe8, 0x11, 0x09, 0x4f, 0x60, 0x1c, 0x08, 0x79, 0x86,
	0x03, 0x0c, 0x3a, 0xc7, 0x49, 0xcc, 0x61, 0xd6, 0x3e, 0x33, 0x83, 0x2e,
	0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00
};

static struct eapol_key_data eapol_key_test_3 = {
	.frame = eapol_key_data_3,
	.frame_len = sizeof(eapol_key_data_3),
	.protocol_version = EAPOL_PROTOCOL_VERSION_2004,
	.packet_len = 95,
	.descriptor_type = EAPOL_DESCRIPTOR_TYPE_80211,
	.key_descriptor_version = EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_SHA1_AES,
	.key_type = true,
	.install = false,
	.key_ack = true,
	.key_mic = false,
	.secure = false,
	.error = false,
	.request = false,
	.encrypted_key_data = false,
	.smk_message = false,
	.key_length = 16,
	.key_replay_counter = 0,
	.key_nonce = { 0xc2, 0xbb, 0x57, 0xab, 0x58, 0x8f, 0x92, 0xeb, 0xbd,
			0x44, 0xe8, 0x11, 0x09, 0x4f, 0x60, 0x1c, 0x08, 0x79,
			0x86, 0x03, 0x0c, 0x3a, 0xc7, 0x49, 0xcc, 0x61, 0xd6,
			0x3e, 0x33, 0x83, 0x2e, 0x50, },
	.eapol_key_iv = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_rsc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_mic_data = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_data_len = 0,
};

/* WPA2 frame, 2 of 4.  For parameters see eapol_4way_test */
static const unsigned char eapol_key_data_4[] = {
	0x01, 0x03, 0x00, 0x75, 0x02, 0x01, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x89, 0xe9, 0x15, 0x65, 0x09, 0x4f,
	0x32, 0x9a, 0x9c, 0xd5, 0x4a, 0x4a, 0x09, 0x0d, 0x2c, 0xf4, 0x34, 0x46,
	0x83, 0xbf, 0x50, 0xef, 0xee, 0x36, 0x08, 0xb6, 0x48, 0x56, 0x80, 0x0e,
	0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc3, 0x1b,
	0x82, 0xff, 0x62, 0xa3, 0x79, 0xb0, 0x8d, 0xd1, 0xfc, 0x82, 0xc2, 0xf7,
	0x68, 0x00, 0x16, 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01,
	0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00,
	0x00
};

static struct eapol_key_data eapol_key_test_4 = {
	.frame = eapol_key_data_4,
	.frame_len = sizeof(eapol_key_data_4),
	.protocol_version = EAPOL_PROTOCOL_VERSION_2001,
	.packet_len = 117,
	.descriptor_type = EAPOL_DESCRIPTOR_TYPE_80211,
	.key_descriptor_version = EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_SHA1_AES,
	.key_type = true,
	.install = false,
	.key_ack = false,
	.key_mic = true,
	.secure = false,
	.error = false,
	.request = false,
	.encrypted_key_data = false,
	.smk_message = false,
	.key_length = 0,
	.key_replay_counter = 0,
	.key_nonce = { 0x32, 0x89, 0xe9, 0x15, 0x65, 0x09, 0x4f, 0x32, 0x9a,
			0x9c, 0xd5, 0x4a, 0x4a, 0x09, 0x0d, 0x2c, 0xf4, 0x34,
			0x46, 0x83, 0xbf, 0x50, 0xef, 0xee, 0x36, 0x08, 0xb6,
			0x48, 0x56, 0x80, 0x0e, 0x84, },
	.eapol_key_iv = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_rsc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_mic_data = { 0x01, 0xc3, 0x1b, 0x82, 0xff, 0x62, 0xa3, 0x79, 0xb0,
				0x8d, 0xd1, 0xfc, 0x82, 0xc2, 0xf7, 0x68 },
	.key_data_len = 22,
};

/* WPA2 frame, 3 of 4.  For parameters see eapol_4way_test */
static const unsigned char eapol_key_data_5[] = {
	0x02, 0x03, 0x00, 0x97, 0x02, 0x13, 0xca, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0xc2, 0xbb, 0x57, 0xab, 0x58, 0x8f, 0x92,
	0xeb, 0xbd, 0x44, 0xe8, 0x11, 0x09, 0x4f, 0x60, 0x1c, 0x08, 0x79, 0x86,
	0x03, 0x0c, 0x3a, 0xc7, 0x49, 0xcc, 0x61, 0xd6, 0x3e, 0x33, 0x83, 0x2e,
	0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf5, 0x35, 0xd9,
	0x18, 0x09, 0x73, 0x1a, 0x1d, 0x29, 0x08, 0x94, 0x70, 0x5e, 0x91, 0x9c,
	0x8e, 0x00, 0x38, 0x19, 0x18, 0xdf, 0x1e, 0xf0, 0xe7, 0x69, 0x66, 0x52,
	0xe2, 0x57, 0x93, 0x80, 0x34, 0xe1, 0x70, 0x38, 0xb9, 0x8b, 0x4c, 0x45,
	0xa9, 0x23, 0xb7, 0xb6, 0xfa, 0x8c, 0x33, 0xe3, 0x7b, 0xdc, 0xd4, 0x7f,
	0xea, 0xb1, 0x1c, 0x22, 0x6a, 0x2c, 0x5e, 0x38, 0xd5, 0xad, 0x79, 0x94,
	0x05, 0xd6, 0x10, 0xa6, 0x95, 0x51, 0xd6, 0x0b, 0xe6, 0x0a, 0x5b,
};

static struct eapol_key_data eapol_key_test_5 = {
	.frame = eapol_key_data_5,
	.frame_len = sizeof(eapol_key_data_5),
	.protocol_version = EAPOL_PROTOCOL_VERSION_2004,
	.packet_len = 151,
	.descriptor_type = EAPOL_DESCRIPTOR_TYPE_80211,
	.key_descriptor_version = EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_SHA1_AES,
	.key_type = true,
	.install = true,
	.key_ack = true,
	.key_mic = true,
	.secure = true,
	.error = false,
	.request = false,
	.encrypted_key_data = true,
	.smk_message = false,
	.key_length = 16,
	.key_replay_counter = 1,
	.key_nonce = { 0xc2, 0xbb, 0x57, 0xab, 0x58, 0x8f, 0x92, 0xeb, 0xbd,
			0x44, 0xe8, 0x11, 0x09, 0x4f, 0x60, 0x1c, 0x08, 0x79,
			0x86, 0x03, 0x0c, 0x3a, 0xc7, 0x49, 0xcc, 0x61, 0xd6,
			0x3e, 0x33, 0x83, 0x2e, 0x50, },
	.eapol_key_iv = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_rsc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_mic_data = { 0xf5, 0x35, 0xd9, 0x18, 0x09, 0x73, 0x1a, 0x1d, 0x29,
				0x08, 0x94, 0x70, 0x5e, 0x91, 0x9c, 0x8e },
	.key_data_len = 56,
};

/* WPA2 frame, 4 of 4.  For parameters see eapol_4way_test */
static const unsigned char eapol_key_data_6[] = {
	0x01, 0x03, 0x00, 0x5f, 0x02, 0x03, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9e, 0x57, 0xa4,
	0xc0, 0x9b, 0xaf, 0xb3, 0x37, 0x5e, 0x46, 0xd3, 0x86, 0xcf, 0x87, 0x27,
	0x53, 0x00, 0x00,
};

static struct eapol_key_data eapol_key_test_6 = {
	.frame = eapol_key_data_6,
	.frame_len = sizeof(eapol_key_data_6),
	.protocol_version = EAPOL_PROTOCOL_VERSION_2001,
	.packet_len = 95,
	.descriptor_type = EAPOL_DESCRIPTOR_TYPE_80211,
	.key_descriptor_version = EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_SHA1_AES,
	.key_type = true,
	.install = false,
	.key_ack = false,
	.key_mic = true,
	.secure = true,
	.error = false,
	.request = false,
	.encrypted_key_data = false,
	.smk_message = false,
	.key_length = 0,
	.key_replay_counter = 1,
	.key_nonce = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.eapol_key_iv = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_rsc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.key_mic_data = { 0x9e, 0x57, 0xa4, 0xc0, 0x9b, 0xaf, 0xb3, 0x37, 0x5e,
				0x46, 0xd3, 0x86, 0xcf, 0x87, 0x27, 0x53, },
	.key_data_len = 0,
};

static void eapol_key_test(const void *data)
{
	const struct eapol_key_data *test = data;
	const struct eapol_key *packet;

	packet = eapol_key_validate(test->frame, test->frame_len);
	assert(packet);

	assert(packet->protocol_version == test->protocol_version);
	assert(packet->packet_type == 0x03);
	assert(L_BE16_TO_CPU(packet->packet_len) == test->packet_len);
	assert(packet->descriptor_type == test->descriptor_type);
	assert(packet->key_descriptor_version == test->key_descriptor_version);
	assert(packet->key_type == test->key_type);
	assert(packet->install == test->install);
	assert(packet->key_ack == test->key_ack);
	assert(packet->key_mic == test->key_mic);
	assert(packet->secure == test->secure);
	assert(packet->error == test->error);
	assert(packet->request == test->request);
	assert(packet->encrypted_key_data == test->encrypted_key_data);
	assert(packet->smk_message == test->smk_message);
	assert(L_BE16_TO_CPU(packet->key_length) == test->key_length);
	assert(L_BE64_TO_CPU(packet->key_replay_counter) ==
			test->key_replay_counter);
	assert(!memcmp(packet->key_nonce, test->key_nonce,
			sizeof(packet->key_nonce)));
	assert(!memcmp(packet->eapol_key_iv, test->eapol_key_iv,
			sizeof(packet->eapol_key_iv)));
	assert(!memcmp(packet->key_mic_data, test->key_mic_data,
			sizeof(packet->key_mic_data)));
	assert(!memcmp(packet->key_rsc, test->key_rsc,
			sizeof(packet->key_rsc)));
	assert(L_BE16_TO_CPU(packet->key_data_len) == test->key_data_len);
}

struct eapol_key_mic_test {
	const uint8_t *frame;
	size_t frame_len;
	enum eapol_key_descriptor_version version;
	const uint8_t *kck;
	const uint8_t *mic;
};

static const uint8_t eapol_key_mic_data_1[] = {
	0x01, 0x03, 0x00, 0x75, 0x02, 0x01, 0x0a, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x59, 0x16, 0x8b, 0xc3, 0xa5, 0xdf, 0x18,
	0xd7, 0x1e, 0xfb, 0x64, 0x23, 0xf3, 0x40, 0x08,
	0x8d, 0xab, 0x9e, 0x1b, 0xa2, 0xbb, 0xc5, 0x86,
	0x59, 0xe0, 0x7b, 0x37, 0x64, 0xb0, 0xde, 0x85,
	0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x16, 0x30, 0x14, 0x01, 0x00, 0x00,
	0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac,
	0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x01,
	0x00,
};

static const uint8_t eapol_key_mic_1[] = {
	0x9c, 0xc3, 0xfa, 0xa0, 0xc6, 0x85, 0x96, 0x1d,
	0x84, 0x06, 0xbb, 0x65, 0x77, 0x45, 0x13, 0x5d,
};

static unsigned char kck_data_1[] = {
	0x9a, 0x75, 0xef, 0x0b, 0xde, 0x7c, 0x20, 0x9c,
	0xca, 0xe1, 0x3f, 0x54, 0xb1, 0xb3, 0x3e, 0xa3,
};

static const struct eapol_key_mic_test eapol_key_mic_test_1 = {
	.frame = eapol_key_mic_data_1,
	.frame_len = sizeof(eapol_key_mic_data_1),
	.version = EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_MD5_ARC4,
	.kck = kck_data_1,
	.mic = eapol_key_mic_1,
};

static const uint8_t eapol_key_mic_2[] = {
	0x6f, 0x04, 0x89, 0xcf, 0x74, 0x06, 0xac, 0xf0,
	0xae, 0x8f, 0xcb, 0x32, 0xbc, 0xe5, 0x7c, 0x37,
};

static const struct eapol_key_mic_test eapol_key_mic_test_2 = {
	.frame = eapol_key_mic_data_1,
	.frame_len = sizeof(eapol_key_mic_data_1),
	.version = EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_SHA1_AES,
	.kck = kck_data_1,
	.mic = eapol_key_mic_2,
};

static void eapol_key_mic_test(const void *data)
{
	const struct eapol_key_mic_test *test = data;
	uint8_t mic[16];

	memset(mic, 0, sizeof(mic));

	switch (test->version) {
	case EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_MD5_ARC4:
		assert(hmac_md5(test->kck, 16, test->frame, test->frame_len,
				mic, 16));
		break;
	case EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_SHA1_AES:
		assert(hmac_sha1(test->kck, 16, test->frame, test->frame_len,
				mic, 16));
		break;
	default:
		assert(false);
	}

	assert(!memcmp(test->mic, mic, sizeof(mic)));
}

struct eapol_calculate_mic_test {
	const uint8_t *frame;
	size_t frame_len;
	const uint8_t *kck;
	const uint8_t *mic;
};

static const struct eapol_calculate_mic_test eapol_calculate_mic_test_1 = {
	.frame = eapol_key_mic_data_1,
	.frame_len = sizeof(eapol_key_mic_data_1),
	.kck = kck_data_1,
	.mic = eapol_key_mic_2,
};

static void eapol_calculate_mic_test(const void *data)
{
	const struct eapol_calculate_mic_test *test = data;
	struct eapol_key *frame;
	uint8_t mic[16];
	bool ret;

	memset(mic, 0, sizeof(mic));
	frame = (struct eapol_key *) test->frame;

	ret = eapol_calculate_mic(test->kck, frame, mic);
	assert(ret);
	assert(!memcmp(test->mic, mic, sizeof(mic)));
}

static void eapol_4way_test(const void *data)
{
	uint8_t anonce[32];
	uint8_t snonce[32];
	uint8_t mic[16];
	struct eapol_key *frame;
	uint8_t aa[] = { 0x24, 0xa2, 0xe1, 0xec, 0x17, 0x04 };
	uint8_t spa[] = { 0xa0, 0xa8, 0xcd, 0x1c, 0x7e, 0xc9 };
	const char *passphrase = "EasilyGuessedPassword";
	const char *ssid = "TestWPA";
	const unsigned char expected_psk[] = {
		0xbf, 0x9a, 0xa3, 0x15, 0x53, 0x00, 0x12, 0x5e,
		0x7a, 0x5e, 0xbb, 0x2a, 0x54, 0x9f, 0x8c, 0xd4,
		0xed, 0xab, 0x8e, 0xe1, 0x2e, 0x94, 0xbf, 0xc2,
		0x4b, 0x33, 0x57, 0xad, 0x04, 0x96, 0x65, 0xd9 };
	unsigned char psk[32];
	struct crypto_ptk *ptk;
	size_t ptk_len;
	bool ret;
	const struct eapol_key *step1;
	const struct eapol_key *step2;
	const struct eapol_key *step3;
	const struct eapol_key *step4;
	uint8_t *decrypted_key_data;

	step1 = eapol_key_validate(eapol_key_data_3,
					sizeof(eapol_key_data_3));
	assert(step1);
	assert(eapol_verify_ptk_1_of_4(step1));
	memcpy(anonce, step1->key_nonce, sizeof(step1->key_nonce));

	step2 = eapol_key_validate(eapol_key_data_4,
					sizeof(eapol_key_data_4));
	assert(eapol_verify_ptk_2_of_4(step2));
	assert(step2);
	memcpy(snonce, step2->key_nonce, sizeof(step2->key_nonce));

	assert(!crypto_psk_from_passphrase(passphrase, (uint8_t *) ssid,
						strlen(ssid), psk));
	assert(!memcmp(expected_psk, psk, sizeof(psk)));

	ptk_len = sizeof(struct crypto_ptk) +
			crypto_cipher_key_len(CRYPTO_CIPHER_CCMP);
	ptk = l_malloc(ptk_len);
	ret = crypto_derive_pairwise_ptk(psk, aa, spa, anonce, snonce,
						ptk, ptk_len);
	assert(ret);

	frame = eapol_create_ptk_2_of_4(EAPOL_PROTOCOL_VERSION_2001,
				EAPOL_KEY_DESCRIPTOR_VERSION_HMAC_SHA1_AES,
				eapol_key_test_4.key_replay_counter,
				snonce, eapol_key_test_4.key_data_len,
				eapol_key_data_4 + sizeof(struct eapol_key));
	assert(frame);
	assert(eapol_calculate_mic(ptk->kck, frame, mic));
	memcpy(frame->key_mic_data, mic, sizeof(mic));
	assert(!memcmp(frame, eapol_key_data_4, sizeof(eapol_key_data_4)));
	l_free(frame);

	step3 = eapol_key_validate(eapol_key_data_5,
					sizeof(eapol_key_data_5));
	assert(step3);
	assert(eapol_verify_ptk_3_of_4(step3));
	assert(!memcmp(anonce, step3->key_nonce, sizeof(step3->key_nonce)));

	assert(eapol_verify_mic(ptk->kck, step3));

	decrypted_key_data = eapol_decrypt_key_data(ptk->kek, step3);
	assert(decrypted_key_data[0] == 48);  // RSNE
	l_free(decrypted_key_data);

	step4 = eapol_key_validate(eapol_key_data_6,
					sizeof(eapol_key_data_6));
	assert(step4);
	assert(eapol_verify_ptk_4_of_4(step4));

	l_free(ptk);
}

int main(int argc, char *argv[])
{
	l_test_init(&argc, &argv);

	l_test_add("/EAPoL Key/Key Frame 1",
			eapol_key_test, &eapol_key_test_1);
	l_test_add("/EAPoL Key/Key Frame 2",
			eapol_key_test, &eapol_key_test_2);
	l_test_add("/EAPoL Key/Key Frame 3",
			eapol_key_test, &eapol_key_test_3);
	l_test_add("/EAPoL Key/Key Frame 4",
			eapol_key_test, &eapol_key_test_4);
	l_test_add("/EAPoL Key/Key Frame 5",
			eapol_key_test, &eapol_key_test_5);
	l_test_add("/EAPoL Key/Key Frame 6",
			eapol_key_test, &eapol_key_test_6);

	l_test_add("/EAPoL Key/MIC Test 1",
			eapol_key_mic_test, &eapol_key_mic_test_1);
	l_test_add("/EAPoL Key/MIC Test 2",
			eapol_key_mic_test, &eapol_key_mic_test_2);

	l_test_add("/EAPoL Key/Calculate MIC Test 1",
			eapol_calculate_mic_test, &eapol_calculate_mic_test_1);

	l_test_add("EAPoL/4-Way Handshake",
			&eapol_4way_test, NULL);

	return l_test_run();
}
