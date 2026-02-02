/*
 * tout.c
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

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <gentyp.h>
#include "sysconf.h"
#include "criterr.h"
#if defined(TERMOUT_SLEEP) && TERMOUT_SLEEP == 1
#include "sleep.h"
#endif
#include "tout.h"
#include <string.h>
#include <stdio.h>

#if TERMOUT == 1

static char buff[TERMOUT_BUFFER_SIZE + 2 * (TERMOUT_MAX_ROW_LENGTH + 1)];
static TaskHandle_t tsk_hndl;
static const char *const tsk_nm = "TOUT";
static void *odv;
static int (*sfn)(void *, void *, int);
#if TERMOUT_SLEEP == 1
static void (*en)(void *);
static void (*dis)(void *);
#endif
static SemaphoreHandle_t mtx;
static QueueHandle_t mque;
static volatile boolean_t ini;
static char *p_bf_st, *p_bf_en, *p_msg_in, *p_msg_out, *p_msg_last;
static int mign_cnt, qfull_cnt, serr_cnt, mprn_cnt, prnerr_cnt;

static void add_msg(const char *fmt, va_list argp);
static void tout_tsk(void *p);
#if TERMOUT_SLEEP == 1
static void sleep_clbk(enum sleep_cmd cmd, ...);
#endif

/**
 * init_tout
 */
#if TERMOUT_SLEEP == 1
void init_tout(struct tout_odev *odev)
#else
void init_tout(int (*p_snd_fn)(void *, void *, int), void *p_odev)
#endif
{
#if TERMOUT_SLEEP == 1
	odv = odev->p_odev;
	sfn = odev->p_snd_fn;
	en = odev->p_en_fn;
	dis = odev->p_dis_fn;
#else
	odv = p_odev;
	sfn = p_snd_fn;
#endif
	p_bf_st = buff;
	memset(p_bf_st, 0xEE, TERMOUT_BUFFER_SIZE);
	p_msg_in = p_bf_st + TERMOUT_BUFFER_SIZE;
	memset(p_msg_in, 0xDD, TERMOUT_MAX_ROW_LENGTH + 1);
	p_msg_out = p_msg_in + TERMOUT_MAX_ROW_LENGTH + 1;
	memset(p_msg_out, 0xCC, TERMOUT_MAX_ROW_LENGTH + 1);
	p_bf_en = p_bf_st + TERMOUT_BUFFER_SIZE - 1;
        mtx = xSemaphoreCreateMutex();
        if (mtx == NULL) {
                crit_err_exit(MALLOC_ERROR);
        }
        mque = xQueueCreate(TERMOUT_MAX_ROWS_IN_QUEUE, sizeof(char *));
        if (mque == NULL) {
                crit_err_exit(MALLOC_ERROR);
        }
        if (pdPASS != xTaskCreate(tout_tsk, tsk_nm, TERMOUT_STACK_SIZE, NULL,
				  TERMOUT_TASK_PRIO, &tsk_hndl)) {
                crit_err_exit(MALLOC_ERROR);
        }
#if TERMOUT_SLEEP == 1
	reg_sleep_clbk(sleep_clbk, SLEEP_PRIO_SUSP_LAST);
#endif
        ini = TRUE;
#if TERMOUT_SEND_CLS_ON_START == 1
	add_msg_tout("\033[2J\033[0;0f");
#endif
}

/**
 * add_msg_tout
 */
void add_msg_tout(const char *fmt, ...)
{
	va_list argp;

        if (!ini) {
                return;
        }
	va_start(argp, fmt);
	add_msg(fmt, argp);
	va_end(argp);
}

/**
 * v_add_msg_tout
 */
void v_add_msg_tout(const char *fmt, va_list argp)
{
        if (!ini) {
                return;
        }
	add_msg(fmt, argp);
}

/**
 * add_msg
 */
static void add_msg(const char *fmt, va_list argp)
{
        int msz, i;
        char *p_new, *p_new0, *p_act, *p_tmp;
	boolean_t ov = FALSE;

	xSemaphoreTake(mtx, portMAX_DELAY);
	msz = vsnprintf(p_msg_in, TERMOUT_MAX_ROW_LENGTH + 1, fmt, argp);
        if (msz < 0) {
		prnerr_cnt++;
                goto exit;
        } else if (msz == 0) {
                goto exit;
	}
        if (msz > TERMOUT_MAX_ROW_LENGTH) {
                msz = TERMOUT_MAX_ROW_LENGTH;
		ov = TRUE;
        }
	if (p_msg_last) {
                if (pdFALSE == xQueuePeek(mque, &p_act, 0)) {
                        p_act = NULL;
#if TERMOUT_SLEEP == 1
		} else {
			if (p_act == NULL) {
				mign_cnt++;
				goto exit;
			}
#endif
		}
		p_new0 = p_new = p_msg_last + *((uint8_t *) p_msg_last);
		if (p_new > p_bf_en) {
			p_new = p_new - p_bf_en - 1 + p_bf_st;
		}
                if (p_act) {
			if (p_msg_last < p_act) {
				if (p_new0 + msz >= p_act) {
					mign_cnt++;
					goto exit;
				}
			} else if (p_msg_last > p_act) {
				if (p_new0 > p_bf_en) {
					if (p_new + msz >= p_act) {
						mign_cnt++;
						goto exit;
					}
				} else {
					if (p_new0 + msz > p_bf_en) {
						ptrdiff_t pd;
						pd = p_new0 + msz - p_bf_en - 1;
						if (p_bf_st + pd >= p_act) {
							mign_cnt++;
							goto exit;
						}
					}
				}
			}
                }
	} else {
		p_new = p_bf_st;
	}
	if (ov) {
		*(p_msg_in + msz - 1) = '\n';
	}
        *((uint8_t *) p_new) = msz + 1;
        p_tmp = p_new + 1;
        for (i = 0; i < msz; i++) {
                if (p_tmp > p_bf_en) {
                        p_tmp = p_bf_st;
                }
                *p_tmp++ = *(p_msg_in + i);
        }
        if (errQUEUE_FULL == xQueueSend(mque, &p_new, 0)) {
                qfull_cnt++;
        } else {
                p_msg_last = p_new;
        }
exit:
	xSemaphoreGive(mtx);
}

/**
 * tout_tsk
 */
static void tout_tsk(void *p)
{
        char *p_m;
        int i, sz;

	add_msg_tout("tout.c: row=%d que=%d buf=%d\n", TERMOUT_MAX_ROW_LENGTH,
	             TERMOUT_MAX_ROWS_IN_QUEUE, TERMOUT_BUFFER_SIZE);
	while (TRUE) {
		xQueuePeek(mque, &p_m, portMAX_DELAY);
#if TERMOUT_SLEEP == 1
		if (p_m == NULL) {
			xQueueReceive(mque, &p_m, 0);
                        vTaskSuspend(NULL);
			continue;
		}
#endif
		sz = *((uint8_t *) p_m);
		sz--;
		p_m++;
		for (i = 0; i < sz; i++) {
                        if (p_m > p_bf_en) {
                                p_m = p_bf_st;
                        }
			*(p_msg_out + i) = *p_m++;
		}
		if (*(p_msg_out + i - 1) == '\n') {
			*(p_msg_out + i - 1) = '\r';
			*(p_msg_out + i) = '\n';
			sz++;
		}
                xQueueReceive(mque, &p_m, 0);
                if (0 != (*sfn)(odv, p_msg_out, sz)) {
			serr_cnt++;
		} else {
			mprn_cnt++;
		}
	}
}

#if TERMOUT_SLEEP == 1
/**
 * sleep_clbk
 */
static void sleep_clbk(enum sleep_cmd cmd, ...)
{
	if (cmd == SLEEP_CMD_SUSP) {
#if SLEEP_LOG_STATE == 1
		add_msg_tout("tout.c: suspend request\n");
		add_msg_tout("-----------------------\n");
#endif
		char *p = NULL;
                xSemaphoreTake(mtx, portMAX_DELAY);
		xQueueSend(mque, &p, portMAX_DELAY);
                xSemaphoreGive(mtx);
		while (eSuspended != eTaskGetState(tsk_hndl)) {
			taskYIELD();
		}
		dis(odv);
	} else {
		en(odv);
		vTaskResume(tsk_hndl);
	}
}
#endif

/**
 * tout_stats
 */
void tout_stats(void)
{
	add_msg_tout("tout.c: mprn=%d mign=%d qfull=%d serr=%d prnerr=%d\n",
	             mprn_cnt, mign_cnt, qfull_cnt, serr_cnt, prnerr_cnt);
}

/**
 * disable_tout
 */
void disable_tout(void)
{
	ini = FALSE;
}

/**
 * tout_tsk_hndl
 */
TaskHandle_t tout_tsk_hndl(void)
{
	return (tsk_hndl);
}

/**
 * tout_mque
 */
QueueHandle_t tout_mque(void)
{
	return (mque);
}
#endif
