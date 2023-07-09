#ifndef TEXTOUT_H
#define TEXTOUT_H


/*
 * DEFINITIONS ________________________________________________________________
 *
 */

#ifdef UNICODE
#define szTextOutCLASS     TEXT("TextOutClassW")
#else
#define szTextOutCLASS     TEXT("TextOutClass")
#endif


/*
 * PROTOTYPES _________________________________________________________________
 *
 */

         void     RegisterTextOutClass       (HINSTANCE);
         void     UnregisterTextOutClass     (void);

#endif

