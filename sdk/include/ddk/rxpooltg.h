#ifndef _RXPOOLTG_H_
#define _RXPOOLTG_H_

#define RX_SRVCALL_POOLTAG ('cSxR')
#define RX_NETROOT_POOLTAG ('rNxR')
#define RX_V_NETROOT_POOLTAG ('nVxR')
#define RX_FCB_POOLTAG ('cFxR')
#define RX_NONPAGEDFCB_POOLTAG ('fNxR')
#define RX_WORKQ_POOLTAG ('qWxR')
#define RX_MISC_POOLTAG ('sMxR')
#define RX_IRPC_POOLTAG ('rIxR')
#ifdef __REACTOS__
#define RX_TLC_POOLTAG ('??xR')
#endif

extern ULONG RxExplodePoolTags;

#define RX_DEFINE_POOLTAG(ExplodedPoolTag, DefaultPoolTag) ((RxExplodePoolTags == 0) ? (DefaultPoolTag) : (ExplodedPoolTag))

#define RX_DIRCTL_POOLTAG RX_DEFINE_POOLTAG('cDxR', RX_MISC_POOLTAG)

#endif
