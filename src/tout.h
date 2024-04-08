/*
 * tout.h
 *
 * Copyright (c) 2021 Jan Rusnak <jan@rusnak.sk>
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

#ifndef TERMOUT_H
#define TERMOUT_H

#if TERMOUT == 1

#ifndef TERMOUT_SLEEP
 #define TERMOUT_SLEEP 0
#endif

#include <stdarg.h>

#if TERMOUT_SLEEP == 1
struct tout_odev {
	void *p_odev;
	int (*p_snd_fn)(void *, void *, int);
        void (*p_en_fn)(void *);
	void (*p_dis_fn)(void *);
};
#endif

/**
 * init_tout
 */
#if TERMOUT_SLEEP == 1
void init_tout(struct tout_odev *odev);
#else
void init_tout(int (*p_snd_fn)(void *, void *, int), void *p_odev);
#endif

/**
 * add_msg_tout
 */
void add_msg_tout(const char *fmt, ...);

/**
 * v_add_msg_tout
 */
void v_add_msg_tout(const char *fmt, va_list argp);

/**
 * tout_stats
 */
void tout_stats(void);

/**
 * disable_tout
 */
void disable_tout(void);

/**
 * tout_tsk_hndl
 */
TaskHandle_t tout_tsk_hndl(void);

/**
 * tout_mque
 */
QueueHandle_t tout_mque(void);
#endif

#endif
