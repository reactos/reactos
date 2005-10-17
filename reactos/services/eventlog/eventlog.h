
#ifndef __EVENTLOG_H__
#define __EVENTLOG_H__

typedef struct _IO_ERROR_LPC
{
    PORT_MESSAGE Header;
    IO_ERROR_LOG_MESSAGE Message;
} IO_ERROR_LPC, *PIO_ERROR_LPC;

BOOL
StartPortThread(VOID);


#endif /* __EVENTLOG_H__ */

/* EOF */
