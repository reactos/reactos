/*
 *  Prototypes and structures for breakpoint handler
 */

BOOL
BPInRange(
    HPRCX         hprc,
    HTHDX         hthd,
    PBREAKPOINT   bp,
    LPADDR        paddrStart,
    DWORD         cb,
    LPDWORD       offset,
    BP_UNIT     * instr
    );

extern  BREAKPOINT* FindProcBP(HPRCX);
extern  BREAKPOINT* FindThreadBP(HTHDX);
extern  BREAKPOINT* FindIdBP(HDEP);
extern  BOOL        RemoveBP(BREAKPOINT*);
extern  BOOL        RemoveAllHprcBP(HPRCX);
extern  void        SetBPFlag(HTHDX, BREAKPOINT*);
extern  BREAKPOINT* AtBP(HTHDX);
extern  void        ClearBPFlag(HTHDX);
extern  void        RestoreInstrBP(HTHDX, BREAKPOINT*);
extern  BREAKPOINT* BPNextHprcPbp( HPRCX hprc, BREAKPOINT * pbp );
extern  BREAKPOINT* BPNextHthdPbp( HTHDX hthd, BREAKPOINT * pbp );

PBREAKPOINT
SetBP(
    HPRCX hprc,
    HTHDX hthd,
    BPTP bptype,
    BPNS bpnotify,
    LPADDR paddr,
    HPID id
    );

PBREAKPOINT
FindBP(
    HPRCX hprc,
    HTHDX hthd,
    BPTP bptype,
    BPNS bpnotify,
    LPADDR paddr,
    BOOL fExact
    );

BOOL
SetBPEx(
      HPRCX         hprc,
      HTHDX         hthd,
      HPID          id,
      DWORD         Count,
      ADDR         *Addrs,
      PBREAKPOINT  *Bps,
      DWORD         ContinueStatus
      );

BOOL
RemoveBPEx(
    DWORD      Count,
    PBREAKPOINT *Bps
    );

PBREAKPOINT
GetNewBp(
    HPRCX         hprc,
    HTHDX         hthd,
    BPTP          BpType,
    BPNS          BpNotify,
    ADDR         *AddrBp,
    HPID          id,
    BREAKPOINT   *BpUse
    );

BOOL
WriteBreakPoint(
    IN PBREAKPOINT Breakpoint
    );

BOOL
RestoreBreakPoint(
    IN PBREAKPOINT Breakpoint
    );

void
AddBpToList(
    PBREAKPOINT pbp
    );

