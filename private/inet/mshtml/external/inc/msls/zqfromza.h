#ifndef ZQFROMZQ_DEFINED
#define ZQFROMZQ_DEFINED

#include "lsdefs.h"

#define zqLim	1491309L				/* higher resolution will overflow */

long ZqFromZa_C (long, long);

#ifdef _X86_

long ZqFromZa_Asm (long, long);
__int64 Div64_Asm (__int64, __int64);
__int64 Mul64_Asm (__int64, __int64);

#define ZqFromZa(dzqInch,za) ZqFromZa_Asm ((dzqInch),(za))

#define Div64(DVND,DVSR) Div64_Asm ((DVND),(DVSR))
#define Mul64(A,B) Mul64_Asm ((A),(B))

#else

#define ZqFromZa(dzqInch,za) ZqFromZa_C ((dzqInch),(za))

#define Div64(DVND,DVSR) ((__int64) (DVND) / (__int64) (DVSR))
#define Mul64(A,B) ((__int64) (A) * (__int64) (B))

#endif

long ZaFromZq(long, long);
long LsLwMultDivR(long, long, long);

#define I_UpFromUa(pdevres,ua)	(ZqFromZa((pdevres)->dxpInch, (ua)))
#define I_UrFromUa(pdevres,ua)	(ZqFromZa((pdevres)->dxrInch, (ua)))
#define I_VpFromVa(pdevres,va)	(ZqFromZa((pdevres)->dypInch, (va)))
#define I_VrFromVa(pdevres,va)	(ZqFromZa((pdevres)->dyrInch, (va)))
#define I_UaFromUp(pdevres,up)	(ZaFromZq((pdevres)->dxpInch, (up)))
#define I_VaFromVp(pdevres,vp)	(ZaFromZq((pdevres)->dypInch, (vp)))
#define I_UaFromUr(pdevres,ur)	(ZaFromZq((pdevres)->dxrInch, (ur)))
#define I_VaFromVr(pdevres,vr)	(ZaFromZq((pdevres)->dyrInch, (vr)))



#define UpFromUa(tfl,pdevres,ua) (((tfl) & fUVertical) ? \
		I_VpFromVa(pdevres,ua) :\
		I_UpFromUa(pdevres,ua) \
)

#define UrFromUa(tfl,pdevres,ua) (((tfl) & fUVertical) ? \
		I_VrFromVa(pdevres,ua) :\
		I_UrFromUa(pdevres,ua) \
)

#define VpFromVa(tfl,pdevres,va) ((tfl) & fUVertical) ? \
		I_UpFromUa(pdevres,va) :\
		I_VpFromVa(pdevres,va) \
)

#define VrFromVa(tfl,pdevres,va) (((tfl) & fUVertical) ? \
		I_UrFromUa(pdevres,va) :\
		I_VrFromVa(pdevres,va) \
)

#define UaFromUp(tfl,pdevres,up) (((tfl) & fUVertical) ? \
		I_VaFromVp(pdevres,up) :\
		I_UaFromUp(pdevres,up) \
)

#define VaFromVp(tfl,pdevres,vp)	(((tfl) & fUVertical) ? \
		I_UaFromUp(pdevres,vp) :\
		I_VaFromVp(pdevres,vp) \
)

#define UaFromUr(tfl,pdevres,ur)	(((tfl) & fUVertical) ?	\
		I_VaFromVr(pdevres,ur) :\
		I_UaFromUr(pdevres,ur) \
)

#define VaFromVr(tfl,pdevres,vr)	(((tfl) & fUVertical) ?	\
		I_UaFromUr(pdevres,vr) :\
		I_VaFromVr(pdevres,vr) \
)

/*
#define UpFromUr(pdevres,ur)	UpFromUa(pdevres, UaFromUr(pdevres, ur))
#define VpFromVr(pdevres,vr)	VpFromVa(pdevres, VaFromVr(pdevres, vr))
*/
#define UpFromUr(tfl,pdevres,ur)	(((tfl) & fUVertical) ? \
								LsLwMultDivR(ur, (pdevres)->dypInch, (pdevres)->dyrInch): \
								LsLwMultDivR(ur, (pdevres)->dxpInch, (pdevres)->dxrInch) \
									)
#define VpFromVr(tfl,pdevres,vr)	(((tfl) & fUVertical) ? \
								LsLwMultDivR(vr, (pdevres)->dxpInch, (pdevres)->dxrInch): \
								LsLwMultDivR(vr, (pdevres)->dypInch, (pdevres)->dyrInch) \
									)
#define UrFromUp(tfl,pdevres,up)	(((tfl) & fUVertical) ? \
								LsLwMultDivR(up, (pdevres)->dyrInch, (pdevres)->dypInch): \
								LsLwMultDivR(up, (pdevres)->dxrInch, (pdevres)->dxpInch) \
									)
#define VrFromVp(tfl,pdevres,vp)	(((tfl) & fUVertical) ? \
								LsLwMultDivR(vp, (pdevres)->dxrInch, (pdevres)->dxpInch): \
								LsLwMultDivR(vp, (pdevres)->dyrInch, (pdevres)->dypInch) \
									)

#endif /* ZQFROMZQ_DEFINED */
