$if (_WDMDDK_)
/** Kernel definitions for IA64 **/

/* Interrupt request levels */
#define PASSIVE_LEVEL           0
#define LOW_LEVEL               0
#define APC_LEVEL               1
#define DISPATCH_LEVEL          2
#define CMC_LEVEL               3
#define DEVICE_LEVEL_BASE       4
#define PC_LEVEL                12
#define IPI_LEVEL               14
#define DRS_LEVEL               14
#define CLOCK_LEVEL             13
#define POWER_LEVEL             15
#define PROFILE_LEVEL           15
#define HIGH_LEVEL              15

#define KI_USER_SHARED_DATA ((ULONG_PTR)(KADDRESS_BASE + 0xFFFE0000))
extern NTKERNELAPI volatile LARGE_INTEGER KeTickCount;

#define PAUSE_PROCESSOR __yield();

FORCEINLINE
VOID
KeFlushWriteBuffer(VOID)
{
  __mf ();
  return;
}

NTSYSAPI
PKTHREAD
NTAPI
KeGetCurrentThread(VOID);

$endif
