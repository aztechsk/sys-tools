/*
 * tools.h
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

#ifndef TOOLS_H
#define TOOLS_H

#ifndef TOOLS_EXTRACT_BITS
 #define TOOLS_EXTRACT_BITS 0
#endif

#ifndef TOOLS_EXTRACT_BITS_LE
 #define TOOLS_EXTRACT_BITS_LE 0
#endif

struct txt_item {
	int idx;
	const char *str;
};

/**
 * find_txt_item
 */
const char *find_txt_item(int val, const struct txt_item *ary, const char *nf);

/**
 * prn_bv_pos
 */
void prn_bv_pos(char *s, unsigned int n, int sz, int msb);

/**
 * prn_bv_str
 */
void prn_bv_str(char *s, unsigned int n, int sz);

/**
 * conv_bv_sz
 */
boolean_t conv_bv_sz(const char *s, unsigned int *n, int sz);

/**
 * num_diff
 */
int num_diff(int n1, int n2);

/**
 * bit_pos
 */
int bit_pos(unsigned int bmp);

/**
 * long_division
 */
unsigned long long long_division(unsigned long long n, unsigned long long d);

enum uint_typename {
	UI8_TYPE = 8,
	UI16_TYPE = 16,
	UI32_TYPE = 32,
	UI64_TYPE = 64
};

#if TOOLS_EXTRACT_BITS == 1
/**
 * extract_bits
 *
 * Extracts bits from memory area organized as array composed of integer
 * types with exact width.
 *
 * @in: Pointer to input memory area (starting address of array).
 * @in_t: Type of input array member (enum uint_typename).
 * @out: Pointer to output memory area (starting address of array).
 *   Output memory area size must be sufficient to store output bits.
 * @out_t: Type of ouput array member (enum uint_typename).
 * @bits_start: Address of bits sequence for extract.
 * @bits_len: Number of bits in extracted sequence.
 */
void extract_bits(void *in, enum uint_typename in_t, void *out, enum uint_typename out_t, int bits_start, int bits_len);
#endif

#if TOOLS_EXTRACT_BITS_LE == 1
/**
 * extract_bits_le (little endian)
 *
 * Extracts bits from memory area organized as array composed of integer
 * types with exact width.
 *
 * @in: Pointer to input memory area (starting address of array).
 * @in_t: Type of input array member (enum uint_typename).
 * @out: Pointer to output memory area (starting address of array).
 *   Output memory area size must be sufficient to store output bits.
 * @out_t: Type of ouput array member (enum uint_typename).
 * @bits_start: Address of bits sequence for extract.
 * @bits_len: Number of bits in extracted sequence.
 */
void extract_bits_le(void *in, enum uint_typename in_t, void *out, enum uint_typename out_t, int bits_start, int bits_len);
#endif

#endif
