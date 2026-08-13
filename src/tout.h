/*
 * tout.h
 *
 * Copyright (c) 2026 Jan Rusnak <jan@rusnak.sk>
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

/**
 * @file tout.h
 * @brief Asynchronous terminal output queue.
 *
 * The terminal output module provides a non-blocking style logging/output path
 * for task context code. A caller formats a message and enqueues it into an
 * internal circular buffer. A dedicated output task later removes queued
 * messages and sends them using a user supplied low-level output function.
 *
 * The caller therefore does not wait for the physical serial/terminal output to
 * complete. It can still block for a short time on the internal mutex while
 * another task is formatting and enqueueing a message. The module is intended
 * for task context only; its public functions must not be called from ISR.
 *
 * A queued message is stored as one length byte followed by message data in the
 * internal circular buffer. Because the length is stored in one byte,
 * TERMOUT_MAX_ROW_LENGTH must not be greater than 254. The circular buffer must
 * be large enough to hold at least two maximum size records:
 *
 * @code
 * TERMOUT_MAX_ROW_LENGTH <= 254
 * TERMOUT_BUFFER_SIZE >= 2 * (TERMOUT_MAX_ROW_LENGTH + 1)
 * @endcode
 *
 * If the message queue or circular buffer is full, the message is dropped and a
 * diagnostic counter is incremented. The caller is not blocked waiting for
 * buffer space. The counters can be printed by tout_stats().
 *
 * When TERMOUT_SLEEP is enabled, the output device can be disabled before sleep
 * and re-enabled after wakeup using callbacks supplied in struct tout_odev.
 */

#ifndef TERMOUT_H
#define TERMOUT_H

#if TERMOUT == 1

#ifndef TERMOUT_SLEEP
 #define TERMOUT_SLEEP 0
#endif

#include <stdarg.h>

#if TERMOUT_SLEEP == 1
/**
 * @brief Terminal output device callbacks for sleep-aware mode.
 *
 * The structure describes one low-level output device. It is passed to
 * init_tout() when TERMOUT_SLEEP is enabled. The send callback is used by the
 * terminal output task to write already formatted message data. The enable and
 * disable callbacks are used by the sleep callback to power or gate the output
 * device around system sleep.
 */
struct tout_odev {
	/** Low-level output device argument passed to callbacks. */
	void *p_odev;
	/**
	 * Send message data.
	 *
	 * The callback is called from the terminal output task context.
	 *
	 * @param dev Low-level output device argument.
	 * @param buf Pointer to message bytes.
	 * @param size Number of bytes to send.
	 *
	 * @return 0 on success, non-zero on output error.
	 */
	int (*p_snd_fn)(void *dev, void *buf, int size);
	/** Enable output device after wakeup. */
	void (*p_en_fn)(void *dev);
	/** Disable output device before sleep. */
	void (*p_dis_fn)(void *dev);
};
#endif

/**
 * @brief Initialize terminal output module.
 *
 * Initializes the internal circular buffer, mutex, message queue and terminal
 * output task. The output task sends queued messages by calling the supplied
 * low-level send function. init_tout() must be called before add_msg_tout() or
 * v_add_msg_tout() can enqueue messages.
 *
 * In normal mode the caller supplies the send function and its device argument
 * directly. In sleep-aware mode the caller supplies struct tout_odev containing
 * send, enable and disable callbacks.
 *
 * Invalid programmer parameters or allocation failures are fatal and are handled
 * by crit_err_exit() in the implementation.
 *
 * This function is task/startup context only and must not be called from ISR.
 *
 * @param odev Output device descriptor used when TERMOUT_SLEEP is enabled.
 * @param p_snd_fn Low-level send function used by the output task when
 * TERMOUT_SLEEP is disabled.
 * @param p_odev Low-level output device argument passed to p_snd_fn when
 * TERMOUT_SLEEP is disabled.
 */
#if TERMOUT_SLEEP == 1
void init_tout(struct tout_odev *odev);
#else
void init_tout(int (*p_snd_fn)(void *, void *, int), void *p_odev);
#endif

/**
 * @brief Format and enqueue terminal output message.
 *
 * Formats a message using printf-style formatting and stores it in the internal
 * terminal output circular buffer. The message is later sent by the terminal
 * output task. The caller does not wait for physical output completion.
 *
 * The message is truncated to TERMOUT_MAX_ROW_LENGTH bytes. If truncation
 * happens, the last stored byte is changed to '\n'. If the queue or circular
 * buffer has no free space, the message is dropped and diagnostic counters are
 * incremented. No error is returned to the caller.
 *
 * The function is serialized by an internal mutex and may block while another
 * task is enqueueing a message. It must not be called from ISR.
 *
 * Calls made before init_tout() completes, or after disable_tout(), are ignored.
 *
 * @param fmt printf-style format string.
 */
void add_msg_tout(const char *fmt, ...);

/**
 * @brief Enqueue terminal output message from a va_list.
 *
 * This is the va_list variant of add_msg_tout(). It has the same buffering,
 * truncation, drop and task-context rules.
 *
 * Calls made before init_tout() completes, or after disable_tout(), are ignored.
 *
 * @param fmt printf-style format string.
 * @param argp Format argument list.
 */
void v_add_msg_tout(const char *fmt, va_list argp);

/**
 * @brief Print terminal output diagnostic counters.
 *
 * Enqueues one diagnostic line with message counters. The counters include
 * successfully printed messages, ignored messages because of buffer pressure,
 * full queue events, low-level send errors and formatting errors.
 *
 * This function uses add_msg_tout() internally and follows the same task-context
 * rules.
 */
void tout_stats(void);

/**
 * @brief Disable terminal output message enqueueing.
 *
 * After this function is called, add_msg_tout() and v_add_msg_tout() ignore new
 * messages. Already queued messages may still be processed by the output task.
 */
void disable_tout(void);

/**
 * @brief Return terminal output task handle.
 *
 * @return FreeRTOS task handle of the terminal output task.
 */
TaskHandle_t tout_tsk_hndl(void);

/**
 * @brief Return terminal output message queue handle.
 *
 * The returned queue contains pointers to messages stored in the internal
 * circular buffer. It is exposed for diagnostics/integration only; normal users
 * should not modify it.
 *
 * @return FreeRTOS queue handle used by the terminal output module.
 */
QueueHandle_t tout_mque(void);
#endif

#endif
