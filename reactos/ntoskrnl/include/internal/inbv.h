#ifndef NTOSKRNL_INBV_H
#define NTOSKRNL_INBV_H

/* INCLUDES ******************************************************************/

/* DEFINES *******************************************************************/

/* FUNCTIONS *****************************************************************/

VOID NTAPI INIT_FUNCTION
InbvDisplayInitialize(VOID);

VOID NTAPI
InbvDisplayInitialize2(BOOLEAN NoGuiBoot);

VOID NTAPI
InbvDisplayBootLogo(VOID);

VOID NTAPI
InbvUpdateProgressBar(ULONG Progress);

VOID NTAPI
InbvFinalizeBootLogo(VOID);

#endif /* NTOSKRNL_INBV_H */


