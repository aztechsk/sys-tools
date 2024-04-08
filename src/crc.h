/*
 * crc.h
 *
 * Copyright (c) 2020 Jan Rusnak <jan@rusnak.sk>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef CRC_H
#define CRC_H

#ifndef CRC_16_FUNC
 #define CRC_16_FUNC 0
#endif
#ifndef CRC_CCIT_FUNC
 #define CRC_CCIT_FUNC 0
#endif

#if CRC_16_FUNC == 1
#define INIT_CRC_16 0x0000U
#define INIT_CRC_16_MODB 0xFFFFU

/**
 * crc_16
 *
 * Compute CRC-16 for data buffer.
 *
 * @crc: Previous CRC value.
 * @buf: Pointer to data buffer.
 * @len: Size of buffer.
 *
 * Returns: Updated CRC value.
 */
uint16_t crc_16(uint16_t crc, const uint8_t *buf, int size);
#endif

#if CRC_CCIT_FUNC == 1
#define INIT_CRC_CCITT 0xFFFFU

/**
 * crc_ccit
 *
 * Compute CRC-CCIT for data buffer.
 *
 * @crc: Previous CRC value.
 * @buf: Pointer to data buffer
 * @len: Size of buffer.
 *
 * Returns: Updated CRC value.
 */
uint16_t crc_ccit(uint16_t crc, const uint8_t *buf, int size);
#endif

#endif
