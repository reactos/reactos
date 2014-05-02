#include <asm.inc>

.code64
.align 4

MACRO(DEFINE_ALIAS, alias, orig)
EXTERN &orig:ABS
ALIAS <&alias> = <&orig>
ENDM

DEFINE_ALIAS ?AllocSysString@CHString@@QBEPA_WXZ, ?AllocSysString@CHString@@QBEPAGXZ
DEFINE_ALIAS ?AssignCopy@CHString@@IAEXHPB_W@Z, ?AssignCopy@CHString@@IAEXHPBG@Z
DEFINE_ALIAS ??0CHString@@QAE@PB_W@Z, ??0CHString@@QAE@PBG@Z
DEFINE_ALIAS ??0CHString@@QAE@PB_WH@Z, ??0CHString@@QAE@PBGH@Z
DEFINE_ALIAS ??0CHString@@QAE@_WH@Z, ??0CHString@@QAE@GH@Z
DEFINE_ALIAS ?Collate@CHString@@QBEHPB_W@Z, ?Collate@CHString@@QBEHPBG@Z
DEFINE_ALIAS ?Compare@CHString@@QBEHPB_W@Z, ?Compare@CHString@@QBEHPBG@Z
DEFINE_ALIAS ?CompareNoCase@CHString@@QBEHPB_W@Z, ?CompareNoCase@CHString@@QBEHPBG@Z
DEFINE_ALIAS ?ConcatCopy@CHString@@IAEXHPB_WH0@Z, ?ConcatCopy@CHString@@IAEXHPBGH0@Z
DEFINE_ALIAS ?ConcatInPlace@CHString@@IAEXHPB_W@Z, ?ConcatInPlace@CHString@@IAEXHPBG@Z
DEFINE_ALIAS ?Find@CHString@@QBEHPB_W@Z, ?Find@CHString@@QBEHPBG@Z
DEFINE_ALIAS ?Find@CHString@@QBEH_W@Z, ?Find@CHString@@QBEHG@Z
DEFINE_ALIAS ?FindOneOf@CHString@@QBEHPB_W@Z, ?FindOneOf@CHString@@QBEHPBG@Z
DEFINE_ALIAS ?Format@CHString@@QAAXPB_WZZ, ?Format@CHString@@QAAXPBGZZ
DEFINE_ALIAS ?FormatMessageW@CHString@@QAAXPB_WZZ, ?FormatMessageW@CHString@@QAAXPBGZZ
DEFINE_ALIAS ?FormatV@CHString@@QAEXPB_WPAD@Z, ?FormatV@CHString@@QAEXPBGPAD@Z
DEFINE_ALIAS ?GetAt@CHString@@QBE_WH@Z, ?GetAt@CHString@@QBEGH@Z
DEFINE_ALIAS ?GetBuffer@CHString@@QAEPA_WH@Z, ?GetBuffer@CHString@@QAEPAGH@Z
DEFINE_ALIAS ?GetBufferSetLength@CHString@@QAEPA_WH@Z, ?GetBufferSetLength@CHString@@QAEPAGH@Z
DEFINE_ALIAS ?LoadStringW@CHString@@IAEHIPA_WI@Z, ?LoadStringW@CHString@@IAEHIPAGI@Z
DEFINE_ALIAS ?LockBuffer@CHString@@QAEPA_WXZ, ?LockBuffer@CHString@@QAEPAGXZ
DEFINE_ALIAS ?ReverseFind@CHString@@QBEH_W@Z, ?ReverseFind@CHString@@QBEHG@Z
DEFINE_ALIAS ?SafeStrlen@CHString@@KGHPB_W@Z, ?SafeStrlen@CHString@@KGHPBG@Z
DEFINE_ALIAS ?SetAt@CHString@@QAEXH_W@Z, ?SetAt@CHString@@QAEXHG@Z
DEFINE_ALIAS ?SpanExcluding@CHString@@QBE?AV1@PB_W@Z, ?SpanExcluding@CHString@@QBE?AV1@PBG@Z
DEFINE_ALIAS ?SpanIncluding@CHString@@QBE?AV1@PB_W@Z, ?SpanIncluding@CHString@@QBE?AV1@PBG@Z
DEFINE_ALIAS ??BCHString@@QBEPB_WXZ, ??BCHString@@QBEPBGXZ
DEFINE_ALIAS ??YCHString@@QAEABV0@PB_W@Z, ??YCHString@@QAEABV0@PBG@Z
DEFINE_ALIAS ??YCHString@@QAEABV0@_W@Z, ??YCHString@@QAEABV0@G@Z
DEFINE_ALIAS ??4CHString@@QAEABV0@PB_W@Z, ??4CHString@@QAEABV0@PBG@Z
DEFINE_ALIAS ??4CHString@@QAEABV0@_W@Z, ??4CHString@@QAEABV0@G@Z
DEFINE_ALIAS ??ACHString@@QBE_WH@Z, ??ACHString@@QBEGH@Z
DEFINE_ALIAS ??H@YG?AVCHString@@_WABV0@@Z, ??H@YG?AVCHString@@GABV0@@Z
DEFINE_ALIAS ??H@YG?AVCHString@@ABV0@_W@Z, ??H@YG?AVCHString@@ABV0@G@Z
DEFINE_ALIAS ??H@YG?AVCHString@@ABV0@PB_W@Z, ??H@YG?AVCHString@@ABV0@PBG@Z
DEFINE_ALIAS ??H@YG?AVCHString@@PB_WABV0@@Z, ??H@YG?AVCHString@@PBGABV0@@Z

END
