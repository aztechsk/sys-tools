/*
 * tsknfo.c
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
#include "msgconf.h"
#include "fmalloc.h"
#include "tsknfo.h"
#include <string.h>

#if TERMOUT == 1 && configUSE_TRACE_FACILITY == 1

/**
 * print_task_info
 */
void print_task_info(void)
{
	char *p_bf, *p_r;
	boolean_t lst = FALSE;
	int i;
	UBaseType_t prio;

	if (NULL == (p_bf = pvPortMalloc(V_TASK_LIST_BUFFER_SIZE))) {
		crit_err_exit(MALLOC_ERROR);
	}
	memset(p_bf, 0xEE, V_TASK_LIST_BUFFER_SIZE);
	vTaskList(p_bf);
	if (*p_bf == 0x00) {
		crit_err_exit(MALLOC_ERROR);
	}
	prio = uxTaskPriorityGet(NULL);
	vTaskPrioritySet(NULL, TASK_PRIO_HIGH);
	msg(INF, "Name\t\tStat\tPrio\tStckFre\tTCBn\n");
	p_r = p_bf;
	while (!lst) {
		i = 0;
		while (*(p_r + i) != '\r') {
			i++;
		}
		if (*(p_r + i + 2) == '\0') {
			lst = TRUE;
		}
		*(p_r + i) = '\n';
		*(p_r + i + 1) = '\0';
		add_msg_tout(p_r);
		p_r += i + 2;
	}
	for (i = 0; i < V_TASK_LIST_BUFFER_SIZE; i++) {
		if (*((unsigned char *) p_bf + i) == 0xEE) {
			break;
		}
	}
	msg(INF, ">>%d>\n", V_TASK_LIST_BUFFER_SIZE - i);
	vTaskPrioritySet(NULL, prio);
	vPortFree(p_bf);
}
#endif

#if configCHECK_FOR_STACK_OVERFLOW != 0
/**
 * vApplicationStackOverflowHook
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
        crit_err_exit(TASK_STACK_OVERFLOW);
}
#endif
