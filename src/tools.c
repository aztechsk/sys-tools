/*
 * tools.c
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

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <gentyp.h>
#include "sysconf.h"
#include "criterr.h"
#include "tools.h"
#include <string.h>

/**
 * find_txt_item
 */
const char *find_txt_item(int val, const struct txt_item *ary, const char *nf)
{
	int i = 0;

	while ((ary + i)->str != NULL) {
		if ((ary + i)->idx == val) {
			return ((ary + i)->str);
		}
		i++;
	}
	return (nf);
}

/**
 * prn_bv_pos
 */
void prn_bv_pos(char *s, unsigned int n, int sz, int msb)
{
	unsigned int m = 1 << (sz - 1);

	for (int i = sz; i > 0; i--) {
		*s = 'b';
		*++s = msb / 10 + '0';
		*++s = msb % 10 + '0';
		*++s = '=';
		if (n & m) {
			*++s = '1';
		} else {
			*++s = '0';
		}
		if (i != 1) {
			*++s = ' ';
		} else {
			*++s = '\0';
		}
		n <<= 1;
		s++;
		msb--;
	}
}

/**
 * prn_bv_str
 */
void prn_bv_str(char *s, unsigned int n, int sz)
{
	unsigned int m = 1 << (sz - 1);

	for (int i = sz; i > 0; i--) {
		if (i != sz && !(i % 4)) {
			*s++ = ' ';
		}
		if (n & m) {
			*s++ = '1';
		} else {
			*s++ = '0';
		}
		if (i == 1) {
			*s++ = '\0';
		}
		n <<= 1;
	}
}

/**
 * conv_bv_sz
 */
boolean_t conv_bv_sz(const char *s, unsigned int *n, int sz)
{
	int bsz = 0;

        *n = 0;
	for (int i = 0; i < (int) strlen(s); i++) {
		if (*(s + i) == '0' || *(s + i) == '1') {
			*n <<= 1;
			if (*(s + i) == '1') {
				*n |= 1;
			}
			bsz++;
		} else if (*(s + i) == ' ') {
			continue;
		} else {
			return (FALSE);
		}
	}
	if (bsz != sz) {
		return (FALSE);
	}
	return (TRUE);
}

/**
 * num_diff
 */
int num_diff(int n1, int n2)
{
	if (n1 >= n2) {
		return (n1 - n2);
	} else {
		return (n2 - n1);
	}
}

/**
 * bit_pos
 */
int bit_pos(unsigned int bmp)
{
	unsigned int cmp = 0x01;
	int pos = 0;

	for (; pos < (int) sizeof(unsigned int) * 8; pos++) {
		if (bmp & cmp) {
			break;
		} else {
			cmp <<= 1;
		}
	}
	return (pos);
}

/**
 * long_division
 */
unsigned long long long_division(unsigned long long n, unsigned long long d)
{
	unsigned long long q = 0, r = 0, bit;

	for (int i = 63; i >= 0; i--) {
		bit = (unsigned long long) 1 << i;
		r = r << 1;
		if (n & bit) {
			r |= 0x01;
		}
		if (r >= d) {
			r = r - d;
			q |= bit;
		}
	}
	return q;
}

#if TOOLS_EXTRACT_BITS == 1
/**
 * extract_bits
 */
void extract_bits(void *in, enum uint_typename in_t, void *out, enum uint_typename out_t, int bits_start, int bits_len)
{
	union {
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
	} v;
	int s_adr, r_sft, l_sft, out_of, len, out_len;
	uint8_t *p8 = out;
	uint16_t *p16 = out;
        uint32_t *p32 = out;
        uint64_t *p64 = out;

	out_len = bits_len / 8;
	if (bits_len % 8) {
		out_len++;
	}
	out_of = 0;
	len = bits_len;
	s_adr = bits_start / in_t;
        r_sft = bits_start % in_t;
	l_sft = in_t - r_sft;
	for (int i = s_adr; len > 0; i++) {
		switch (in_t) {
		case UI8_TYPE :
			v.u8 = *((uint8_t *) in + i);
			if (r_sft) {
				v.u8 >>= r_sft;
				if (len > l_sft) {
					v.u8 |= *((uint8_t *) in + i + 1) << l_sft;
				}
			}
			if (len <= in_t) {
				uint8_t m = 1 << 7;
				for (int k = 0; k < in_t - len; k++) {
					v.u8 &= ~m;
					m >>= 1;
				}
				len = 0;
			} else {
				len -= in_t;
			}
			switch (out_t) {
			case UI8_TYPE :
                                *p8++ = v.u8;
				break;
			case UI16_TYPE :
				if (out_of == 0) {
					*p16 = v.u8;
					out_of = 1;
				} else if (out_of == 1) {
					*p16 |= v.u8 << 8;
                                        p16++;
					out_of = 0;
				}
				break;
			case UI32_TYPE :
				if (out_of == 0) {
					*p32 = v.u8;
					out_of = 1;
				} else if (out_of == 1) {
					*p32 |= v.u8 << 8;
					out_of = 2;
				} else if (out_of == 2) {
					*p32 |= v.u8 << 16;
					out_of = 3;
				} else if (out_of == 3) {
					*p32 |= v.u8 << 24;
                                        p32++;
					out_of = 0;
				}
				break;
			case UI64_TYPE :
				if (out_of == 0) {
					*p64 = v.u8;
					out_of = 1;
				} else if (out_of == 1) {
					*p64 |= v.u8 << 8;
					out_of = 2;
				} else if (out_of == 2) {
					*p64 |= v.u8 << 16;
					out_of = 3;
				} else if (out_of == 3) {
					*p64 |= v.u8 << 24;
					out_of = 4;
				} else if (out_of == 4) {
					*p64 |= (uint64_t) v.u8 << 32;
					out_of = 5;
				} else if (out_of == 5) {
					*p64 |= (uint64_t) v.u8 << 40;
					out_of = 6;
				} else if (out_of == 6) {
					*p64 |= (uint64_t) v.u8 << 48;
					out_of = 7;
				} else if (out_of == 7) {
					*p64 |= (uint64_t) v.u8 << 56;
                                        p64++;
					out_of = 0;
				}
				break;
			default :
				crit_err_exit(BAD_PARAMETER);
				break;
			}
			break;
                case UI16_TYPE :
                	v.u16 = *((uint16_t *) in + i);
			if (r_sft) {
				v.u16 >>= r_sft;
				if (len > l_sft) {
					v.u16 |= *((uint16_t *) in + i + 1) << l_sft;
				}
			}
			if (len <= in_t) {
				uint16_t m = 1 << 15;
				for (int k = 0; k < in_t - len; k++) {
					v.u16 &= ~m;
					m >>= 1;
				}
				len = 0;
			} else {
				len -= in_t;
			}
			switch (out_t) {
			case UI8_TYPE :
				for (int j = 0; j < 2; j++) {
					if (out_len) {
						*p8++ = v.u16 >> 8 * j;
                                                out_len--;
					} else {
						break;
					}
				}
				break;
			case UI16_TYPE :
				*p16++ = v.u16;
				break;
			case UI32_TYPE :
				if (out_of == 0) {
					*p32 = v.u16;
					out_of = 1;
				} else if (out_of == 1) {
					*p32 |= v.u16 << 16;
					p32++;
					out_of = 0;
				}
				break;
			case UI64_TYPE :
				if (out_of == 0) {
					*p64 = v.u16;
					out_of = 1;
				} else if (out_of == 1) {
					*p64 |= v.u16 << 16;
					out_of = 2;
				} else if (out_of == 2) {
					*p64 |= (uint64_t) v.u16 << 32;
					out_of = 3;
				} else if (out_of == 3) {
					*p64 |= (uint64_t) v.u16 << 48;
					p64++;
					out_of = 0;
				}
				break;
			default :
				crit_err_exit(BAD_PARAMETER);
				break;
			}
			break;
                case UI32_TYPE :
                	v.u32 = *((uint32_t *) in + i);
			if (r_sft) {
				v.u32 >>= r_sft;
				if (len > l_sft) {
					v.u32 |= *((uint32_t *) in + i + 1) << l_sft;
				}
			}
			if (len <= in_t) {
				uint32_t m = 1 << 31;
				for (int k = 0; k < in_t - len; k++) {
					v.u32 &= ~m;
					m >>= 1;
				}
				len = 0;
			} else {
				len -= in_t;
			}
			switch (out_t) {
			case UI8_TYPE :
				for (int j = 0; j < 4; j++) {
					if (out_len) {
						*p8++ = v.u32 >> 8 * j;
                                                out_len--;
					} else {
						break;
					}
				}
				break;
			case UI16_TYPE :
				for (int j = 0; j < 2; j++) {
					if (out_len) {
						*p16++ = v.u32 >> 16 * j;
                                                out_len -= 2;
					} else {
						break;
					}
				}
				break;
			case UI32_TYPE :
				*p32++ = v.u32;
				break;
			case UI64_TYPE :
				if (out_of == 0) {
					*p64 = v.u32;
					out_of = 1;
				} else if (out_of == 1) {
					*p64 |= (uint64_t) v.u32 << 32;
                                        p64++;
					out_of = 0;
				}
				break;
			default :
				crit_err_exit(BAD_PARAMETER);
				break;
			}
			break;
                case UI64_TYPE :
                	v.u64 = *((uint64_t *) in + i);
			if (r_sft) {
				v.u64 >>= r_sft;
				if (len > l_sft) {
					v.u64 |= *((uint64_t *) in + i + 1) << l_sft;
				}
			}
			if (len <= in_t) {
				uint64_t m = (uint64_t) 1 << 63;
				for (int k = 0; k < in_t - len; k++) {
					v.u64 &= ~m;
					m >>= 1;
				}
				len = 0;
			} else {
				len -= in_t;
			}
			switch (out_t) {
			case UI8_TYPE :
				for (int j = 0; j < 8; j++) {
					if (out_len) {
						*p8++ = v.u64 >> 8 * j;
                                                out_len--;
					} else {
						break;
					}
				}
				break;
			case UI16_TYPE :
				for (int j = 0; j < 4; j++) {
					if (out_len) {
						*p16++ = v.u64 >> 16 * j;
						out_len -= 2;
					} else {
						break;
					}
				}
				break;
			case UI32_TYPE :
				for (int j = 0; j < 2; j++) {
					if (out_len) {
						*p32++ = v.u64 >> 32 * j;
						out_len -= 4;
					} else {
						break;
					}
				}
				break;
			case UI64_TYPE :
				*p64++ = v.u64;
				break;
			default :
				crit_err_exit(BAD_PARAMETER);
				break;
			}
			break;
		default :
			crit_err_exit(BAD_PARAMETER);
			break;
		}
	}
}
#endif

#if TOOLS_EXTRACT_BITS_LE == 1
/**
 * extract_bits_le
 */
void extract_bits_le(void *in, enum uint_typename in_t, void *out, enum uint_typename out_t, int bits_start, int bits_len)
{
	int s_adr, r_sft, l_sft, out_n;
	uint8_t *p_out, v;

	s_adr = bits_start / 8;
        r_sft = bits_start % 8;
	l_sft = 8 - r_sft;
	p_out = out;
	out_n = bits_len / out_t;
	if (bits_len % out_t) {
		out_n++;
	}
	switch (out_t) {
	case UI8_TYPE :
		break;
	case UI16_TYPE :
		*((uint16_t *) out + out_n - 1) = 0;
		break;
	case UI32_TYPE :
		*((uint32_t *) out + out_n - 1) = 0;
		break;
	case UI64_TYPE :
		*((uint64_t *) out + out_n - 1) = 0;
		break;
	default :
		crit_err_exit(BAD_PARAMETER);
		break;
	}
        for (int i = s_adr; bits_len > 0; i++) {
		v = *((uint8_t *) in + i);
		if (r_sft) {
			v >>= r_sft;
                        if (bits_len > l_sft) {
				v |= *((uint8_t *) in + i + 1) << l_sft;
			}
		}
		if (bits_len <= 8) {
			uint8_t m = 1 << 7;
			for (int k = 0; k < 8 - bits_len; k++) {
				v &= ~m;
				m >>= 1;
			}
			bits_len = 0;
		} else {
			bits_len -= 8;
		}
                *p_out++ = v;
	}
}
#endif
