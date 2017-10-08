#ifndef KJK_PSEH2_H_
#define KJK_PSEH2_H_


#define _SEH2_TRY if(1) {
#define _SEH2_EXCEPT(...) } if(0) {
#define _SEH2_END }
#define _SEH2_YIELD(STMT_) STMT_
#define _SEH2_LEAVE
#define _SEH2_FINALLY } if(1) {
#define _SEH2_GetExceptionInformation() (GetExceptionInformation())
#define _SEH2_GetExceptionCode() (0)
#define _SEH2_AbnormalTermination() (0)

struct _EXCEPTION_RECORD;
struct _EXCEPTION_POINTERS;
struct _CONTEXT;

typedef int (__cdecl * _SEH2FrameHandler_t)
(
	struct _EXCEPTION_RECORD *,
	void *,
	struct _CONTEXT *,
	void *
);

typedef struct __SEH2Registration
{
	struct __SEH2Registration * SER_Prev;
	_SEH2FrameHandler_t SER_Handler;
}
_SEH2Registration_t;


#endif

/* EOF */
