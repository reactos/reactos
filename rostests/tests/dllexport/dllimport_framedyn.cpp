
#include <chstring.h>

class CHString2 : CHString
{
public:
    void
    UseStuff(void)
    {
        AllocBeforeWrite(1);// ?AllocBeforeWrite@CHString@@IAEXH@Z(long)
        AllocBuffer(1);// ?AllocBuffer@CHString@@IAEXH@Z(long)
        AllocCopy(*this, 1, 2, 3);// ?AllocCopy@CHString@@IBEXAAV1@HHH@Z(ptr long long long)
        AllocSysString(); // ?AllocSysString@CHString@@QBEPAGXZ()
        AssignCopy(1, L"test");// ?AssignCopy@CHString@@IAEXHPBG@Z(long wstr)
// ??0CHString@@QAE@ABV0@@Z(ptr)
// ??0CHString@@QAE@PBD@Z(str)
// ??0CHString@@QAE@PBE@Z(str)
// ??0CHString@@QAE@PBG@Z(wstr)
// ??0CHString@@QAE@PBGH@Z(wstr long)
// ??0CHString@@QAE@GH@Z(long long)
// ??0CHString@@QAE@XZ()
        Collate(L"test");// ?Collate@CHString@@QBEHPBG@Z(wstr)
        Compare(L"test");// ?Compare@CHString@@QBEHPBG@Z(wstr)
        CompareNoCase(L"test");// ?CompareNoCase@CHString@@QBEHPBG@Z(wstr)
        ConcatCopy(1, L"test", 2, L"data");// ?ConcatCopy@CHString@@IAEXHPBGH0@Z(long wstr long wstr)
        ConcatInPlace(1, L"test");// ?ConcatInPlace@CHString@@IAEXHPBG@Z(long wstr)
        CopyBeforeWrite();// ?CopyBeforeWrite@CHString@@IAEXXZ()
        Empty();// ?Empty@CHString@@QAEXXZ()
        Find(L"test");// ?Find@CHString@@QBEHPBG@Z(wstr)
        Find(L'a');// ?Find@CHString@@QBEHG@Z(long)
        FindOneOf(L"abc");// ?FindOneOf@CHString@@QBEHPBG@Z(wstr)
        Format(1, 123);// ?Format@CHString@@QAAXIZZ(long long long)
        Format(L"Format %x", 123);// ?Format@CHString@@QAAXPBGZZ(long wstr long)
        FormatMessageW(1, 123);// ?FormatMessageW@CHString@@QAAXIZZ(long long long)
        FormatMessageW(L"Format %x", 123);// ?FormatMessageW@CHString@@QAAXPBGZZ(long ptr long)
        FormatV(L"Format %x", NULL);// ?FormatV@CHString@@QAEXPBGPAD@Z(wstr long)
        FreeExtra();// ?FreeExtra@CHString@@QAEXXZ()
        GetAllocLength();// ?GetAllocLength@CHString@@QBEHXZ()
        GetAt(0);// ?GetAt@CHString@@QBEGH@Z(long)
        GetBuffer(0);// ?GetBuffer@CHString@@QAEPAGH@Z(long)
        GetBufferSetLength(1);// ?GetBufferSetLength@CHString@@QAEPAGH@Z(long)
        (void)*(volatile int*)GetData();// ?GetData@CHString@@IBEPAUCHStringData@@XZ()
        GetLength();// ?GetLength@CHString@@QBEHXZ()
        Init();// ?Init@CHString@@IAEXXZ()
        IsEmpty();// ?IsEmpty@CHString@@QBEHXZ()
        Left(1);// ?Left@CHString@@QBE?AV1@H@Z(long)
        LoadStringW(1);// ?LoadStringW@CHString@@QAEHI@Z(long)
        LoadStringW(1, NULL, 256);// ?LoadStringW@CHString@@IAEHIPAGI@Z(long wstr long)
        LockBuffer();// ?LockBuffer@CHString@@QAEPAGXZ()
        MakeLower();// ?MakeLower@CHString@@QAEXXZ()
        MakeReverse();// ?MakeReverse@CHString@@QAEXXZ()
        MakeUpper();// ?MakeUpper@CHString@@QAEXXZ()
        Mid(12);// ?Mid@CHString@@QBE?AV1@H@Z(long)
        Mid(12, 4);// ?Mid@CHString@@QBE?AV1@HH@Z(long long)
        Release(NULL);// ?Release@CHString@@KGXPAUCHStringData@@@Z(ptr)
        Release();// ?Release@CHString@@IAEXXZ()
        ReleaseBuffer(); // ?ReleaseBuffer@CHString@@QAEXH@Z(long)
        ReverseFind(L'a');// ?ReverseFind@CHString@@QBEHG@Z(long)
        Right(2);// ?Right@CHString@@QBE?AV1@H@Z(long)
        SafeStrlen(L"test");// ?SafeStrlen@CHString@@KGHPBG@Z(wstr)
        SetAt(0, L'a');// ?SetAt@CHString@@QAEXHG@Z(long long)
        SpanExcluding(L"test");// ?SpanExcluding@CHString@@QBE?AV1@PBG@Z(long wstr)
        SpanIncluding(L"test");// ?SpanIncluding@CHString@@QBE?AV1@PBG@Z(long wstr)
        TrimLeft();// ?TrimLeft@CHString@@QAEXXZ()
        TrimRight();// ?TrimRight@CHString@@QAEXXZ()
        UnlockBuffer();// ?UnlockBuffer@CHString@@QAEXXZ()
// ??BCHString@@QBEPBGXZ(ptr)
// ??YCHString@@QAEABV0@ABV0@@Z(ptr)
// ??YCHString@@QAEABV0@D@Z(long)
// ??YCHString@@QAEABV0@PBG@Z(wstr)
// ??YCHString@@QAEABV0@G@Z(long)
// ??4CHString@@QAEABV0@PAV0@@Z(ptr)
// ??4CHString@@QAEABV0@ABV0@@Z(ptr)
// ??4CHString@@QAEABV0@PBD@Z(str)
// ??4CHString@@QAEABV0@D@Z(long)
// ??4CHString@@QAEABV0@PBE@Z(str)
// ??4CHString@@QAEABV0@PBG@Z(wstr)
// ??4CHString@@QAEABV0@G@Z(long)
// ??ACHString@@QBEGH@Z(long)
// ??1CHString@@QAE@XZ()
// ??H@YG?AVCHString@@GABV0@@Z(long ptr)
// ??H@YG?AVCHString@@ABV0@G@Z(ptr long)
// ??H@YG?AVCHString@@ABV0@PBG@Z(ptr wstr)
// ??H@YG?AVCHString@@PBGABV0@@Z(wstr ptr)
// ??H@YG?AVCHString@@ABV0@0@Z(ptr ptr)

    }
};

int
test()
{
    CHString2 String;

    String.UseStuff();

    return 0;
}
