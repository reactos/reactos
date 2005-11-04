/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_fifo.h,v 1.2 2002/02/22 21:32:58 dawes Exp $ */

#ifndef _FFB_FIFO_H
#define _FFB_FIFO_H

#define FFBFifo(__fmesa, __n) \
do {	ffbScreenPrivate *__fScrn = (__fmesa)->ffbScreen; \
	int __cur_slots = __fScrn->fifo_cache; \
	if ((__cur_slots - (__n)) < 0) { \
		ffb_fbcPtr __ffb = __fmesa->regs; \
		do { __cur_slots = (((int)__ffb->ucsr & FFB_UCSR_FIFO_MASK) - 4); \
		} while ((__cur_slots - (__n)) < 0); \
	} (__fScrn)->fifo_cache = (__cur_slots - (__n)); \
} while(0)

#define FFBWait(__fmesa, __ffb) \
do {	ffbScreenPrivate *__fScrn = (__fmesa)->ffbScreen; \
	if (__fScrn->rp_active) { \
		unsigned int __regval = (__ffb)->ucsr; \
		while((__regval & FFB_UCSR_ALL_BUSY) != 0) { \
			__regval = (__ffb)->ucsr; \
		} \
		__fScrn->fifo_cache = ((int)(__regval & FFB_UCSR_FIFO_MASK)) - 4; \
		__fScrn->rp_active = 0; \
	} \
} while(0)

#endif /* !(_FFB_FIFO_H) */
