
TEST_NAMEX(init)
{
    CPathX test1;
    CPathX test2(_X("C:\\SomePath\\.\\That\\..\\is.txt.exe.bin"));
    CPathX test3(test2);

    ok(CStringX("").Compare(test1) == 0, "Expected test1 to be '', was: '%s'\n", dbgstrx(test1));
    ok(CStringX("C:\\SomePath\\.\\That\\..\\is.txt.exe.bin").Compare(test2) == 0, "Expected test2 to be 'C:\\SomePath\\.\\That\\..\\is.txt.exe.bin', was: '%s'\n", dbgstrx(test2));
    ok(CStringX("C:\\SomePath\\.\\That\\..\\is.txt.exe.bin").Compare(test3) == 0, "Expected test3 to be 'C:\\SomePath\\.\\That\\..\\is.txt.exe.bin', was: '%s'\n", dbgstrx(test3));

    test1 = _X("test");
    test2 = _X("test");
    ok(CStringX("test").Compare(test1) == 0, "Expected test1 to be 'test', was: '%s'\n", dbgstrx(test1));
    ok(CStringX("test").Compare(test2) == 0, "Expected test2 to be 'test', was: '%s'\n", dbgstrx(test1));


#if 0
    // this does not compile:
    test3 = test1 + _X("test");
    test3 = test1 + test2;
    CPathX test4(test1 + test2);
#endif
    // This one compiles, but does not behave as wanted!
    CPathX test5(test1 + _X("test"));
    ok(CStringX("testtest").Compare(test5) == 0, "Expected test5 to be 'testtest', was: '%s'\n", dbgstrx(test1));
}

TEST_NAMEX(modify)
{
    CPathX test(_X("C:\\Some Path\\.\\That\\..\\is.txt.exe.bin"));
    CPathX canon;
    CPathX empty;

    test.Canonicalize();
    ok(CStringX("C:\\Some Path\\is.txt.exe.bin").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path\\is.txt.exe.bin', was: '%s'\n", dbgstrx(test));

    canon.Canonicalize();
    ok(CStringX("\\").Compare(canon) == 0,
        "Expected canon to be '\\', was: '%s'\n", dbgstrx(canon));

    ok(test.FileExists() == FALSE, "FileExists succeeded for '%s'\n", dbgstrx(test));

    int ext = test.FindExtension();
    ok(ext == 23, "Expected ext to be 23, was: %i\n", ext);
    ext = empty.FindExtension();
    ok(ext == -1, "Expected ext to be -1, was: %i\n", ext);

    int name = test.FindFileName();
    ok(name == 13, "Expected name to be 13, was: %i\n", name);
    name = empty.FindFileName();
    ok(name == -1, "Expected name to be -1, was: %i\n", name);

    int drive = test.GetDriveNumber();
    ok(drive == 2, "Expected drive to be 2, was: %i\n", drive);
    drive = empty.GetDriveNumber();
    ok(drive == -1, "Expected drive to be -1, was: %i\n", drive);

    int skiproot = test.SkipRoot();
    ok(skiproot == 3, "Expected skiproot to be 3, was: %i\n", skiproot);

#if 0
    // Does not handle an empty string correctly
    skiproot = empty.SkipRoot();
    ok(skiproot == -1, "Expected skiproot to be -1, was: %i\n", skiproot);
#endif


    test.RemoveExtension();
    ok(CStringX("C:\\Some Path\\is.txt.exe").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path\\is.txt.exe', was: '%s'\n", dbgstrx(test));

    empty.RemoveExtension();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    CStringX extName = test.GetExtension();
    ok(CStringX(".exe").Compare(extName) == 0,
        "Expected test.GetExtension() to be '.exe', was: '%s'\n", dbgstrx(extName));

    extName = empty.GetExtension();
    ok(CStringX("").Compare(extName) == 0,
        "Expected empty.GetExtension() to be '', was: '%s'\n", dbgstrx(extName));

    test.QuoteSpaces();
    ok(CStringX("\"C:\\Some Path\\is.txt.exe\"").Compare(test) == 0,
        "Expected test to be '\"C:\\Some Path\\is.txt.exe\"', was: '%s'\n", dbgstrx(test));

    empty.QuoteSpaces();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    ok(test.RemoveFileSpec(), "RemoveFileSpec failed\n");
    ok(CStringX("\"C:\\Some Path").Compare(test) == 0,
        "Expected test to be '\"C:\\Some Path', was: '%s'\n", dbgstrx(test));

    ok(empty.RemoveFileSpec() == FALSE, "RemoveFileSpec succeeded\n");
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    test.UnquoteSpaces();
    ok(CStringX("\"C:\\Some Path").Compare(test) == 0,
        "Expected test to be '\"C:\\Some Path\\is.txt.exe\"', was: '%s'\n", dbgstrx(test));

    empty.UnquoteSpaces();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    test.m_strPath += _X('"');

    test.UnquoteSpaces();
    ok(CStringX("C:\\Some Path").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path', was: '%s'\n", dbgstrx(test));

    empty.UnquoteSpaces();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    ok(test.AddExtension(_X(".dummy")), "AddExtension failed\n");
    ok(CStringX("C:\\Some Path.dummy").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.dummy', was: '%s'\n", dbgstrx(test));

    ok(empty.AddExtension(_X(".dummy")), "AddExtension failed\n");
    ok(CStringX(".dummy").Compare(empty) == 0,
        "Expected empty to be '.dummy', was: '%s'\n", dbgstrx(empty));
    empty = _X("");

    test.AddBackslash();
    ok(CStringX("C:\\Some Path.dummy\\").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.dummy\\', was: '%s'\n", dbgstrx(test));

    empty.AddBackslash();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    test.AddBackslash();
    ok(CStringX("C:\\Some Path.dummy\\").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.dummy\\', was: '%s'\n", dbgstrx(test));

    empty.AddBackslash();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    test.RemoveBlanks();
    ok(CStringX("C:\\Some Path.dummy\\").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.dummy\\', was: '%s'\n", dbgstrx(test));

    empty.RemoveBlanks();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    test = _X(" C:\\Some Path.dummy\\   ");
    empty = _X("      ");

    test.RemoveBlanks();
    ok(CStringX("C:\\Some Path.dummy\\").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.dummy\\', was: '%s'\n", dbgstrx(test));

    empty.RemoveBlanks();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    test.RemoveBackslash();
    ok(CStringX("C:\\Some Path.dummy").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.dummy', was: '%s'\n", dbgstrx(test));

    empty.RemoveBackslash();
    ok(CStringX("").Compare(empty) == 0,
        "Expected empty to be '', was: '%s'\n", dbgstrx(empty));

    ok(test.RenameExtension(_X(".txt")), "RenameExtension failed\n");
    ok(CStringX("C:\\Some Path.txt").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.txt', was: '%s'\n", dbgstrx(test));

    ok(empty.RenameExtension(_X(".txt")), "RenameExtension failed\n");
    ok(CStringX(".txt").Compare(empty) == 0,
        "Expected empty to be '.txt', was: '%s'\n", dbgstrx(empty));

    empty = _X("");

    test += _X("something");
    ok(CStringX("C:\\Some Path.txt\\something").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.txt', was: '%s'\n", dbgstrx(test));

    empty += _X("something");
    ok(CStringX("something").Compare(empty) == 0,
        "Expected empty to be 'something', was: '%s'\n", dbgstrx(empty));

    ok(test.Append(_X("stuff.txt")), "Append failed\n");
    ok(CStringX("C:\\Some Path.txt\\something\\stuff.txt").Compare(test) == 0,
        "Expected test to be 'C:\\Some Path.txt\\something\\stuff.txt', was: '%s'\n", dbgstrx(test));

    ok(empty.Append(_X("")), "Append failed\n");
    ok(CStringX("something").Compare(empty) == 0,
        "Expected empty to be 'something', was: '%s'\n", dbgstrx(empty));

    ok(test.MatchSpec(_X("stuff.txt")) == FALSE, "MatchSpec succeeded for 'stuff.txt'\n");
    ok(test.MatchSpec(_X("stuff.exe")) == FALSE, "MatchSpec succeeded for 'stuff.exe'\n");
    ok(test.MatchSpec(_X(".txt")) == FALSE, "MatchSpec succeeded for '.txt'\n");
    ok(test.MatchSpec(_X("txt")) == FALSE, "MatchSpec succeeded for 'txt'\n");
    ok(test.MatchSpec(_X("*.txt")), "MatchSpec failed for '*.txt'\n");
    ok(test.MatchSpec(_X("C:\\*.txt")), "MatchSpec failed for 'C:\\*.txt'\n");
    ok(test.MatchSpec(_X("D:\\*.txt")) == FALSE, "MatchSpec succeeded for 'D:\\*.txt'\n");

    CStringX same("C:\\Some Path.txt\\something\\stuff.txt");

    const CStringX& constref = test;
    ok(same.Compare(constref) == 0,
        "Expected constref to be '%s', was: '%s'\n", dbgstrx(same), dbgstrx(constref));


    CStringX& ref = test;
    ok(same.Compare(ref) == 0,
        "Expected ref to be '%s', was: '%s'\n", dbgstrx(same), dbgstrx(ref));

    const XCHAR* rawptr = test;
    ok(same.Compare(rawptr) == 0,
        "Expected rawptr to be '%s', was: '%s'\n", dbgstrx(same), dbgstrx(rawptr));

    test.BuildRoot(5);
    ok(CStringX("F:\\").Compare(test) == 0,
        "Expected test to be 'F:\\', was: '%s'\n", dbgstrx(test));

#if 0
    // Asserts 'iDrive <= 25'
    test.BuildRoot(105);
    ok(CStringX("f:").Compare(test) == 0,
        "Expected test to be 'f:', was: '%s'\n", dbgstrx(test));
#endif

    test.Combine(_X("C:"), _X("test"));
    ok(CStringX("C:\\test").Compare(test) == 0,
        "Expected test to be 'C:\\test', was: '%s'\n", dbgstrx(test));

    test.Combine(_X("C:\\"), _X("\\test\\"));
    ok(CStringX("C:\\test\\").Compare(test) == 0,
        "Expected test to be 'C:\\test\\', was: '%s'\n", dbgstrx(test));

    test.Combine(_X("C:\\"), _X("\\test\\\\"));
    ok(CStringX("C:\\test\\\\").Compare(test) == 0,
        "Expected test to be 'C:\\test\\\\', was: '%s'\n", dbgstrx(test));
}

TEST_NAMEX(is_something)
{
    XCHAR Buffer[MAX_PATH] = { 0 };
    GetModuleFileNameX(NULL, Buffer, MAX_PATH);

    CPathX test(Buffer);

    ok(test.IsDirectory() == FALSE, "IsDirectory succeeded for '%s'\n", dbgstrx(test));
    ok(test.IsFileSpec() == FALSE, "IsFileSpec succeeded for '%s'\n", dbgstrx(test));
    ok(test.IsRelative() == FALSE, "IsRelative succeeded for '%s'\n", dbgstrx(test));
    ok(test.IsRoot() == FALSE, "IsRoot succeeded for '%s'\n", dbgstrx(test));
    ok(test.FileExists(), "FileExists failed for '%s'\n", dbgstrx(test));

    test.StripPath();
    ok(test.IsFileSpec(), "IsFileSpec failed for '%s'\n", dbgstrx(test));
    ok(test.IsRelative(), "IsRelative failed for '%s'\n", dbgstrx(test));
    ok(test.IsRoot() == FALSE, "IsRoot succeeded for '%s'\n", dbgstrx(test));


    ok(test.StripToRoot() == FALSE, "StripToRoot succeeded for '%s'\n", dbgstrx(test));
    ok(CStringX("").Compare(test) == 0,
        "Expected test to be '', was: '%s'\n", dbgstrx(test));

    test = Buffer;

    ok(test.StripToRoot(), "StripToRoot failed for '%s'\n", dbgstrx(test));
    ok(test.IsRoot(), "IsRoot failed for '%s'\n", dbgstrx(test));

    {
        CStringX help = test;
        ok(help.GetLength() == 3, "IsRoot weird result for '%s'\n", dbgstrx(help));
    }

    test = Buffer;

    test.RemoveFileSpec();
    ok(test.IsDirectory(), "IsDirectory failed for '%s'\n", dbgstrx(test));
}

