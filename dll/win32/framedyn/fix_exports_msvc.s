#include <asm.inc>

.code64
.align 4

MACRO(DEFINE_ALIAS, alias, orig)
EXTERN &orig:ABS
ALIAS <&alias> = <&orig>
ENDM

DEFINE_ALIAS ?AllocSysString@CHString@@QBEPAGXZ, ?AllocSysString@CHString@@QBEPA_WXZ
DEFINE_ALIAS ?AssignCopy@CHString@@IAEXHPBG@Z, ?AssignCopy@CHString@@IAEXHPB_W@Z
DEFINE_ALIAS ??0CHString@@QAE@PBG@Z, ??0CHString@@QAE@PB_W@Z
DEFINE_ALIAS ??0CHString@@QAE@PBGH@Z, ??0CHString@@QAE@PB_WH@Z
DEFINE_ALIAS ??0CHString@@QAE@GH@Z, ??0CHString@@QAE@_WH@Z
DEFINE_ALIAS ?Collate@CHString@@QBEHPBG@Z, ?Collate@CHString@@QBEHPB_W@Z
DEFINE_ALIAS ?Compare@CHString@@QBEHPBG@Z, ?Compare@CHString@@QBEHPB_W@Z
DEFINE_ALIAS ?CompareNoCase@CHString@@QBEHPBG@Z, ?CompareNoCase@CHString@@QBEHPB_W@Z
DEFINE_ALIAS ?ConcatCopy@CHString@@IAEXHPBGH0@Z, ?ConcatCopy@CHString@@IAEXHPB_WH0@Z
DEFINE_ALIAS ?ConcatInPlace@CHString@@IAEXHPBG@Z, ?ConcatInPlace@CHString@@IAEXHPB_W@Z
DEFINE_ALIAS ?Find@CHString@@QBEHPBG@Z, ?Find@CHString@@QBEHPB_W@Z
DEFINE_ALIAS ?Find@CHString@@QBEHG@Z, ?Find@CHString@@QBEH_W@Z
DEFINE_ALIAS ?FindOneOf@CHString@@QBEHPBG@Z, ?FindOneOf@CHString@@QBEHPB_W@Z
DEFINE_ALIAS ?Format@CHString@@QAAXPBGZZ, ?Format@CHString@@QAAXPB_WZZ
DEFINE_ALIAS ?FormatMessageW@CHString@@QAAXPBGZZ, ?FormatMessageW@CHString@@QAAXPB_WZZ
DEFINE_ALIAS ?FormatV@CHString@@QAEXPBGPAD@Z, ?FormatV@CHString@@QAEXPB_WPAD@Z
DEFINE_ALIAS ?GetAt@CHString@@QBEGH@Z, ?GetAt@CHString@@QBE_WH@Z
DEFINE_ALIAS ?GetBuffer@CHString@@QAEPAGH@Z, ?GetBuffer@CHString@@QAEPA_WH@Z
DEFINE_ALIAS ?GetBufferSetLength@CHString@@QAEPAGH@Z, ?GetBufferSetLength@CHString@@QAEPA_WH@Z
DEFINE_ALIAS ?LoadStringW@CHString@@IAEHIPAGI@Z, ?LoadStringW@CHString@@IAEHIPA_WI@Z
DEFINE_ALIAS ?LockBuffer@CHString@@QAEPAGXZ, ?LockBuffer@CHString@@QAEPA_WXZ
DEFINE_ALIAS ?ReverseFind@CHString@@QBEHG@Z, ?ReverseFind@CHString@@QBEH_W@Z
DEFINE_ALIAS ?SafeStrlen@CHString@@KGHPBG@Z, ?SafeStrlen@CHString@@KGHPB_W@Z
DEFINE_ALIAS ?SetAt@CHString@@QAEXHG@Z, ?SetAt@CHString@@QAEXH_W@Z
DEFINE_ALIAS ?SpanExcluding@CHString@@QBE?AV1@PBG@Z, ?SpanExcluding@CHString@@QBE?AV1@PB_W@Z
DEFINE_ALIAS ?SpanIncluding@CHString@@QBE?AV1@PBG@Z, ?SpanIncluding@CHString@@QBE?AV1@PB_W@Z
DEFINE_ALIAS ??BCHString@@QBEPBGXZ, ??BCHString@@QBEPB_WXZ
DEFINE_ALIAS ??YCHString@@QAEABV0@PBG@Z, ??YCHString@@QAEABV0@PB_W@Z
DEFINE_ALIAS ??YCHString@@QAEABV0@G@Z, ??YCHString@@QAEABV0@_W@Z
DEFINE_ALIAS ??4CHString@@QAEABV0@PBG@Z, ??4CHString@@QAEABV0@PB_W@Z
DEFINE_ALIAS ??4CHString@@QAEABV0@G@Z, ??4CHString@@QAEABV0@_W@Z
DEFINE_ALIAS ??ACHString@@QBEGH@Z, ??ACHString@@QBE_WH@Z
DEFINE_ALIAS ??H@YG?AVCHString@@GABV0@@Z, ??H@YG?AVCHString@@_WABV0@@Z
DEFINE_ALIAS ??H@YG?AVCHString@@ABV0@G@Z, ??H@YG?AVCHString@@ABV0@_W@Z
DEFINE_ALIAS ??H@YG?AVCHString@@ABV0@PBG@Z, ??H@YG?AVCHString@@ABV0@PB_W@Z
DEFINE_ALIAS ??H@YG?AVCHString@@PBGABV0@@Z, ??H@YG?AVCHString@@PB_WABV0@@Z

END
