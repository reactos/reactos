#pragma  once
#define ARM_ARCH_TIMER_ENABLE   (1 << 0)
#define ARM_ARCH_TIMER_IMASK    (1 << 1)
#define ARM_ARCH_TIMER_ISTATUS  (1 << 2)

UINT32
ArmReadCntFrq (
  VOID
  );

VOID
ArmWriteCntFrq (
  UINT32  FreqInHz
  );

UINT64
ArmReadCntPct (
  VOID
  );

UINT32
ArmReadCntkCtl (
  VOID
  );

VOID
ArmWriteCntkCtl (
  UINT32  Val
  );

UINT32
ArmReadCntpTval (
  VOID
  );

VOID
ArmWriteCntpTval (
  UINT32  Val
  );

UINT32
ArmReadCntpCtl (
  VOID
  );

VOID
ArmWriteCntpCtl (
  UINT32  Val
  );

UINT32
ArmReadCntvTval (
  VOID
  );

VOID
ArmWriteCntvTval (
  UINT32  Val
  );

UINT32
ArmReadCntvCtl (
  VOID
  );

VOID
ArmWriteCntvCtl (
  UINT32  Val
  );

UINT64
ArmReadCntvCt (
  VOID
  );

UINT64
ArmReadCntpCval (
  VOID
  );

VOID
ArmWriteCntpCval (
  UINT64  Val
  );

UINT64
ArmReadCntvCval (
  VOID
  );

VOID
ArmWriteCntvCval (
  UINT64  Val
  );

UINT64
ArmReadCntvOff (
  VOID
  );

VOID
ArmWriteCntvOff (
  UINT64  Val
  );

UINT32
ArmGetPhysicalAddressBits (
  VOID
  );