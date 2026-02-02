/*
 * tin.c
 *
 * Copyright (c) 2024 Jan Rusnak <jan@rusnak.sk>
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
#if defined(TERMIN_SLEEP) && TERMIN_SLEEP == 1
#include "sleep.h"
#include "hwerr.h"
#endif
#include "tin.h"
#include <string.h>

#if TERMIN == 1

#if TERMOUT != 1
 #error "tin.c depends on tou.c"
#endif

static char buff[TERMIN_MAX_ROW_LENGTH + 1];
static int pos;
static uint8_t c[] = " ";

static TaskHandle_t tsk_hndl;
static const char *const tsk_nm = "TIN";
static boolean_t rcve;
#if TERMIN_START_ECHO_ON == 1
static boolean_t echo = TRUE;
#else
static boolean_t echo;
#endif
static void *idv;
static int (*rfn)(void *, void *, TickType_t);
#if TERMIN_SLEEP == 1
static boolean_t (*intr)(void *);
#endif
static void (*lp)(char *);

static void tin_tsk(void *p);
static void parse_byte(void);
#if TERMIN_SLEEP == 1
static void sleep_clbk(enum sleep_cmd cmd, ...);
#endif

/**
 * init_tin
 */
#if TERMIN_SLEEP == 1
void init_tin(struct tin_idev *idev, void (*lp_fn)(char *))
#else
void init_tin(int (*p_rcv_fn)(void *, void *, TickType_t),
              void *p_idev, void (*lp_fn)(char *))
#endif
{
#if TERMIN_SLEEP == 1
	idv = idev->p_idev;
	rfn = idev->p_rcv_fn;
	intr = idev->p_intr_fn;
#else
	idv = p_idev;
	rfn = p_rcv_fn;
#endif
	lp = lp_fn;
        if (pdPASS != xTaskCreate(tin_tsk, tsk_nm, TERMIN_STACK_SIZE, NULL,
				  TERMIN_TASK_PRIO, &tsk_hndl)) {
                crit_err_exit(MALLOC_ERROR);
        }
#if TERMIN_SLEEP == 1
	reg_sleep_clbk(sleep_clbk, SLEEP_PRIO_SUSP_FIRST);
#endif
}

/**
 * tin_tsk
 */
static void tin_tsk(void *p)
{
	msg(INF, "tin.c: row=%d\n", TERMIN_MAX_ROW_LENGTH);
	while (TRUE) {
#if TERMIN_SLEEP == 1
		int ret;
		if (0 == (ret = (*rfn)(idv, c, portMAX_DELAY))) {
#else
		if (0 == (*rfn)(idv, c, portMAX_DELAY)) {
#endif
			if (*c == '\r') {
				if (!rcve) {
					if (echo) {
						add_msg_tout("\n");
					}
					if (lp) {
						(*lp)(buff);
					}
				} else {
					if (echo) {
						add_msg_tout("\r\nserial line error\n");
					} else {
						add_msg_tout("serial line error\n");
					}
					rcve = FALSE;
				}
				pos = 0;
				memset(buff, 0, TERMIN_MAX_ROW_LENGTH);
				continue;
			}
			parse_byte();
		} else {
#if TERMIN_SLEEP == 1
			if (ret == -EINTR) {
#if SLEEP_LOG_STATE == 1
				add_msg_tout("tin.c: %s suspended\n", tsk_nm);
#endif
				vTaskSuspend(NULL);
#if SLEEP_LOG_STATE == 1
				add_msg_tout("tin.c: %s resumed\n", tsk_nm);
#endif
				continue;
			}
#endif
			rcve = TRUE;
		}
	}
}

/**
 * parse_byte
 */
static void parse_byte(void)
{
	switch (*c) {
	case '\177' :
		/* FALLTHRU */
	case '\010' :
		if (echo && pos) {
			*(buff + --pos) = 0;
			*c = '\177';
			add_msg_tout((const char *) c);
		}
		return;
	case '\022' :
		if (!echo) {
			echo = TRUE;
			rcve = FALSE;
			pos = 0;
			memset(buff, 0, TERMIN_MAX_ROW_LENGTH);
			add_msg_tout("echo on\n");
		}
		return;
	case '\003' :
		rcve = FALSE;
		pos = 0;
		memset(buff, 0, TERMIN_MAX_ROW_LENGTH);
		add_msg_tout("<ETX>\n");
		return;
	case '\033' :
		/* FALLTHRU */
	case '\233' :
		*c = '^';
		break;
	case '\014' :
		if (echo) {
			add_msg_tout("\033[2J\033[0;0f");
			if (pos) {
				add_msg_tout(buff);
			}
		}
		return;
	default :
		if (*c < '\040' || *c > '\176') {
			rcve = FALSE;
			pos = 0;
			memset(buff, 0, TERMIN_MAX_ROW_LENGTH);
			if (echo) {
				add_msg_tout("\r\ninput not 7-bit ASCII\n");
			} else {
				add_msg_tout("input not 7-bit ASCII\n");
			}
			return;
		}
		break;
	}
	if (pos < TERMIN_MAX_ROW_LENGTH) {
		*(buff + pos++) = *c;
		if (echo) {
			add_msg_tout((const char *) c);
		}
	} else {
		rcve = FALSE;
		pos = 0;
		memset(buff, 0, TERMIN_MAX_ROW_LENGTH);
		if (echo) {
			add_msg_tout("\r\nexceeded max line length\n");
		} else {
			add_msg_tout("exceeded max line length\n");
		}
	}
}

#if TERMIN_SLEEP == 1
/**
 * sleep_clbk
 */
static void sleep_clbk(enum sleep_cmd cmd, ...)
{
	if (cmd == SLEEP_CMD_SUSP) {
		while (!intr(idv)) {
			taskYIELD();
		}
		while (eSuspended != eTaskGetState(tsk_hndl)) {
			taskYIELD();
		}
	} else {
		vTaskResume(tsk_hndl);
	}
}
#endif

#endif
