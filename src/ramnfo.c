/*
 * ramnfo.c
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
#include "msgconf.h"
#include "ramnfo.h"

#if TERMOUT == 1

extern unsigned char __stack_start__, __stack_end__;
extern unsigned char __stack_process_start__, __stack_process_end__;

static int st_us(unsigned char *s_st, unsigned char *s_en);

/**
 * print_stack_usage
 */
void print_stack_usage(void)
{
        int us, sz;

        us = st_us(&__stack_start__, &__stack_end__);
        sz = &__stack_end__ - &__stack_start__;
        msg(INF, "main_stack: size=%d used=%d\n", sz, us);

        us = st_us(&__stack_process_start__, &__stack_process_end__);
        sz = &__stack_process_end__ - &__stack_process_start__;
        msg(INF, "proc_stack: size=%d used=%d\n", sz, us);
}

/**
 * st_us
 */
static int st_us(unsigned char *s_st, unsigned char *s_en)
{
	int sz;

        for (sz = s_en - s_st; sz > 0; sz--) {
                if (*s_st++ != 0xCC) {
                        break;
                }
        }
        return (sz);
}

extern unsigned char __heap_start__, __heap_end__;

/**
 * print_heap_usage
 */
void print_heap_usage(void)
{
        int hs;
        UBaseType_t prio;
        unsigned int *p_h;

        hs = &__heap_end__ - &__heap_start__;
	prio = uxTaskPriorityGet(NULL);
	vTaskPrioritySet(NULL, TASK_PRIO_HIGH);
        p_h = (unsigned int *) &__heap_start__;
        msg(INF, "heap size=%d\n", hs);
	msg(INF, "heap free blocks:\n");
        while (p_h) {
                msg(INF, "@%p size=%d\n", p_h, *(p_h + 1));
                p_h = (unsigned int *) *p_h;
        }
        vTaskPrioritySet(NULL, prio);
}
#endif
