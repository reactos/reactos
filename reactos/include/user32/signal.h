/* loader/signal.c */
WINBOOL SIGNAL_Init(void);
void SIGNAL_SetHandler( int sig, void (*func)(), int flags );
void SIGNAL_MaskAsyncEvents( BOOL flag );