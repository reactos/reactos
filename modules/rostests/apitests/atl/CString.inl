
TEST_NAMEX(operators_init)
{
    CStringX test;
    ok(test.IsEmpty() == true, "Expected test to be empty\n");
    ok(test.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", test.GetLength());
    ok(test.GetAllocLength() == 0, "Expected GetAllocLength() to be 0, was: %i\n", test.GetAllocLength());

    // Operator
    const XCHAR* cstring = (const XCHAR*)test;
    ok(cstring != NULL, "Expected a valid pointer\n");
    if (cstring)
    {
        ok(cstring[0] == '\0', "Expected \\0, got: %c (%i)\n", cstring[0], (int)cstring[0]);
    }

    CStringX first(_X("First "));
    ok(first.IsEmpty() != true, "Expected first to not be empty\n");
    ok(first.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", first.GetLength());
    ok_int(first.GetAllocLength(), 6);

    CStringX second(_X("Second"));
    ok(second.IsEmpty() != true, "Expected second to not be empty\n");
    ok(second.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", second.GetLength());
    ok_int(second.GetAllocLength(), 6);

    test = first;
    ok(test.IsEmpty() != true, "Expected test to not be empty\n");
    ok(test.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 6);

    test.Empty();
    ok(test.IsEmpty() == true, "Expected test to be empty\n");
    ok(test.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 0);

    test = _X("First ");
    ok(test.IsEmpty() != true, "Expected test to not be empty\n");
    ok(test.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 6);

    test += second;
    ok(test.IsEmpty() != true, "Expected test to not be empty\n");
    ok(test.GetLength() == 12, "Expected GetLength() to be 12, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 12);

    test = first + second;
    ok(test.IsEmpty() != true, "Expected test to not be empty\n");
    ok(test.GetLength() == 12, "Expected GetLength() to be 12, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 12);

    test = first + second + _X(".");
    ok(test.IsEmpty() != true, "Expected test to not be empty\n");
    ok(test.GetLength() == 13, "Expected GetLength() to be 13, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 13);

    CStringX test2(test);
    ok(test2.IsEmpty() != true, "Expected test2 to not be empty\n");
    ok(test2.GetLength() == 13, "Expected GetLength() to be 13, was: %i\n", test2.GetLength());
    ok_int(test2.GetAllocLength(), 13);

    // Clear it again
    test.Empty();
    ok(test.IsEmpty() == true, "Expected test to be empty\n");
    ok(test.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 0);

    // Assign string
    test = "First ";
    ok(test.IsEmpty() != true, "Expected test to not be empty\n");
    ok(test.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 6);

    CStringA testA = test;
    ok(testA.IsEmpty() != true, "Expected testA to not be empty\n");
    ok(testA.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", testA.GetLength());
    ok_int(testA.GetAllocLength(), 6);

    CStringW testW = test;
    ok(testW.IsEmpty() != true, "Expected testW to not be empty\n");
    ok(testW.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", testW.GetLength());
    ok_int(testW.GetAllocLength(), 6);

    // Assign wstring
    test = L"First ";
    ok(test.IsEmpty() != true, "Expected test to not be empty\n");
    ok(test.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", test.GetLength());
    ok_int(test.GetAllocLength(), 6);
}


TEST_NAMEX(compare)
{
    CStringX s1, s2;
    s1 = s2 = _X("some text 1!");

    int c = s1.Compare(s2);
    ok(c == 0, "Expected c to be 1, was: %i\n", c);

    c = s1.Compare(_X("r"));
    ok(c == 1, "Expected c to be 1, was: %i\n", c);

    c = s1.Compare(_X("s"));
    ok(c == 1, "Expected c to be 1, was: %i\n", c);

    c = s1.Compare(_X("t"));
    ok(c == -1, "Expected c to be -1, was: %i\n", c);

    c = s1.Compare(_X("R"));
    ok(c == 1, "Expected c to be 1, was: %i\n", c);

    c = s1.Compare(_X("S"));
    ok(c == 1, "Expected c to be 1, was: %i\n", c);

    c = s1.Compare(_X("T"));
    ok(c == 1, "Expected c to be 1, was: %i\n", c);

    ok(s1 == s2, "Expected s1 and s2 to be equal: '%s' == '%s'\n", dbgstrx(s1), dbgstrx(s2));

    s1.MakeUpper();  // Does not modify s2
    ok(s1[0] == _X('S'), "Expected s1[0] to be S, was: %c\n", (char)s1[0]);
    ok(s2[0] == _X('s'), "Expected s2[0] to be s, was: %c\n", (char)s2[0]);

    ok(s1 == _X("SOME TEXT 1!"), "Expected s1 to be 'SOME TEXT 1!', was: %s\n", dbgstrx(s1));

    CStringX s3 = s1.MakeLower();
    ok(s1 == _X("some text 1!"), "Expected s1 to be 'some text 1!', was: %s\n", dbgstrx(s1));
    ok(s1 == s3, "Expected s1 and s3 to be equal: '%s' == '%s'\n", dbgstrx(s1), dbgstrx(s3));
}


TEST_NAMEX(find)
{
    CStringX s(_X("adbcdef"));
    int n = s.Find(_X('c'));
    ok(n == 3, "Expected n to be 2, was %i\n", n);
    n = s.Find(_X("de"));
    ok(n == 4, "Expected n to be 4, was %i\n", n);

    CStringX str(_X("The waves are still"));
    n = str.Find(_X('e'), 5);
    ok(n == 7, "Expected n to be 7, was %i\n", n);
    n = str.Find(_X('e'), 7);
    ok(n == 7, "Expected n to be 7, was %i\n", n);

    s = _X("abcdefGHIJKLMNop");
    n = s.FindOneOf(_X("Nd"));
    ok(n == 3, "Expected n to be 3, was %i\n", n);
    n = s.FindOneOf(_X("Np"));
    ok(n == 13, "Expected n to be 13, was %i\n", n);

    n = str.ReverseFind(_X('l'));
    ok(n == 18, "Expected n to be 18, was %i\n", n);

    n = str.ReverseFind(_X('e'));
    ok(n == 12, "Expected n to be 12, was %i\n", n);
}


void WriteString(const XCHAR* pstrFormat, ...)
{
    CStringX str;

    va_list args;
    va_start(args, pstrFormat);

    str.FormatV(pstrFormat, args);
    va_end(args);

    ok(str == _X("10e 1351l"), "Expected str to be '10e 1351l', was: %s\n", dbgstrx(str));
}

TEST_NAMEX(format)
{
    CStringX str;

    str.Format(_X("FP: %.2f"), 12345.12345);
    ok(str == _X("FP: 12345.12"), "Expected str to be 'FP: 12345.12', was: %s\n", dbgstrx(str));

    str.Format(_X("int: %.6d"), 35);
    ok(str == _X("int: 000035"), "Expected str to be 'int: 000035', was: %s\n", dbgstrx(str));

    WriteString(_X("%de %dl"), 10, 1351);
}


TEST_NAMEX(substr)
{
    CStringX s(_X("abcdef"));

    CStringX m = s.Mid(2, 3);
    ok(m == _X("cde"), "Expected str to be 'cde', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 3, "Expected GetLength() to be 3, was: %i\n", m.GetLength());

    m = s.Mid(-5, 3);
    ok(m == _X("abc"), "Expected str to be 'abc', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 3, "Expected GetLength() to be 3, was: %i\n", m.GetLength());

    m = s.Mid(3, 20);
    ok(m == _X("def"), "Expected str to be 'def', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 3, "Expected GetLength() to be 3, was: %i\n", m.GetLength());

    m = s.Mid(3, -1);
    ok(m == _X(""), "Expected str to be '', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", m.GetLength());

    m = s.Mid(2);
    ok(m == _X("cdef"), "Expected str to be 'cdef', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 4, "Expected GetLength() to be 4, was: %i\n", m.GetLength());

    m = s.Mid(-3);
    ok(m == _X("abcdef"), "Expected str to be 'abcdef', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", m.GetLength());

    m = s.Mid(20);
    ok(m == _X(""), "Expected str to be '', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", m.GetLength());

    m = s.Left(2);
    ok(m == _X("ab"), "Expected str to be 'ab', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 2, "Expected GetLength() to be 2, was: %i\n", m.GetLength());

    m = s.Left(40);
    ok(m == _X("abcdef"), "Expected str to be 'abcdef', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", m.GetLength());

    m = s.Left(-10);
    ok(m == _X(""), "Expected str to be '', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", m.GetLength());

    m = s.Right(2);
    ok(m == _X("ef"), "Expected str to be 'ef', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 2, "Expected GetLength() to be 2, was: %i\n", m.GetLength());

    m = s.Right(-40);
    ok(m == _X(""), "Expected str to be '', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", m.GetLength());

    m = s.Right(99);
    ok(m == _X("abcdef"), "Expected str to be 'abcdef', was: %s\n", dbgstrx(m));
    ok(m.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", m.GetLength());
}


TEST_NAMEX(replace)
{
    CStringX str(_X("abcde"));
    int n = str.Replace(_X("b"), _X("bx"));
    ok(n == 1, "Expected n to be 1, was %i\n", n);
    ok(str == _X("abxcde"), "Expected str to be 'abxcde', was: %s\n", dbgstrx(str));
    ok(str.GetLength() == 6, "Expected GetLength() to be 6, was: %i\n", str.GetLength());

    CStringX strBang(_X("The quick brown fox is lazy today of all days"));

    n = strBang.Replace(_X("is"), _X("was"));
    ok(n == 1, "Expected n to be 1, was %i\n", n);
    ok(strBang == _X("The quick brown fox was lazy today of all days"),
        "Expected str to be 'The quick brown fox was lazy today of all days', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 46, "Expected GetLength() to be 46, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X("is"), _X("was"));
    ok(n == 0, "Expected n to be 0, was %i\n", n);
    ok(strBang == _X("The quick brown fox was lazy today of all days"),
        "Expected str to be 'The quick brown fox was lazy today of all days', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 46, "Expected GetLength() to be 46, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X('o'), _X('0'));
    ok(n == 4, "Expected n to be 4, was %i\n", n);
    ok(strBang == _X("The quick br0wn f0x was lazy t0day 0f all days"),
        "Expected str to be 'The quick br0wn f0x was lazy t0day 0f all days', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 46, "Expected GetLength() to be 46, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X('o'), _X('0'));
    ok(n == 0, "Expected n to be 0, was %i\n", n);
    ok(strBang == _X("The quick br0wn f0x was lazy t0day 0f all days"),
        "Expected str to be 'The quick br0wn f0x was lazy t0day 0f all days', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 46, "Expected GetLength() to be 46, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X("y "), _X("y, "));
    ok(n == 2, "Expected n to be 2, was %i\n", n);
    ok(strBang == _X("The quick br0wn f0x was lazy, t0day, 0f all days"),
        "Expected str to be 'The quick br0wn f0x was lazy, t0day, 0f all days', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 48, "Expected GetLength() to be 48, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X(", 0f all days"), _X(""));
    ok(n == 1, "Expected n to be 1, was %i\n", n);
    ok(strBang == _X("The quick br0wn f0x was lazy, t0day"),
        "Expected str to be 'The quick br0wn f0x was lazy, t0day', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 35, "Expected GetLength() to be 35, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X(" lazy, "), _X(" fast "));
    ok(n == 1, "Expected n to be 1, was %i\n", n);
    ok(strBang == _X("The quick br0wn f0x was fast t0day"),
        "Expected str to be 'The quick br0wn f0x was fast t0day', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 34, "Expected GetLength() to be 34, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X("The "), _X(""));
    ok(n == 1, "Expected n to be 1, was %i\n", n);
    ok(strBang == _X("quick br0wn f0x was fast t0day"),
        "Expected str to be 'quick br0wn f0x was fast t0day', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 30, "Expected GetLength() to be 30, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X(" t0day"), _X(""));
    ok(n == 1, "Expected n to be 1, was %i\n", n);
    ok(strBang == _X("quick br0wn f0x was fast"),
        "Expected str to be 'quick br0wn f0x was fast', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 24, "Expected GetLength() to be 24, was: %i\n", strBang.GetLength());

    n = strBang.Replace(_X("quick"), _X("The fast, quick"));
    ok(n == 1, "Expected n to be 1, was %i\n", n);
    ok(strBang == _X("The fast, quick br0wn f0x was fast"),
        "Expected str to be 'The fast, quick br0wn f0x was fast', was: %s\n", dbgstrx(strBang));
    ok(strBang.GetLength() == 34, "Expected GetLength() to be 34, was: %i\n", strBang.GetLength());
}


TEST_NAMEX(trim)
{
    CStringX str;
    str = _X(" \t\r\n******Trim some text!?!?!?!?!\n\r\t ");

    str.TrimLeft();
    ok(str == _X("******Trim some text!?!?!?!?!\n\r\t "), "Expected str to be '******Trim some text!?!?!?!?!\n\r\t ', was: %s\n", dbgstrx(str));
    ok(str.GetLength() == 33, "Expected GetLength() to be 33, was: %i\n", str.GetLength());

    str.TrimRight();
    ok(str == _X("******Trim some text!?!?!?!?!"), "Expected str to be '******Trim some text!?!?!?!?!', was: %s\n", dbgstrx(str));
    ok(str.GetLength() == 29, "Expected GetLength() to be 29, was: %i\n", str.GetLength());

    CStringX str2 = str.Trim(_X("?!*"));
    ok(str2 == _X("Trim some text"), "Expected str to be 'Trim some text', was: %s\n", dbgstrx(str2));
    ok(str2.GetLength() == 14, "Expected GetLength() to be 14, was: %i\n", str2.GetLength());

    str = _X("\t\t   ****Trim some text!");
    str2 = str.TrimLeft(_X("\t *"));
    ok(str2 == _X("Trim some text!"), "Expected str to be 'Trim some text!', was: %s\n", dbgstrx(str2));
    ok(str2.GetLength() == 15, "Expected GetLength() to be 15, was: %i\n", str2.GetLength());

    str = _X("Trim some text!?!?!?!?!");
    str2 = str.TrimRight(_X("?!"));
    ok(str2 == _X("Trim some text"), "Expected str to be 'Trim some text', was: %s\n", dbgstrx(str2));
    ok(str2.GetLength() == 14, "Expected GetLength() to be 14, was: %i\n", str2.GetLength());

    str = _X("\t\t\t\t\t");
    str2 = str.TrimLeft();
    ok(str2 == _X(""), "Expected str2 to be '', was: %s\n", dbgstrx(str2));
    ok(str2.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", str2.GetLength());

    str = _X("\t\t\t\t\t");
    str2 = str.TrimRight();
    ok(str2 == _X(""), "Expected str2 to be '', was: %s\n", dbgstrx(str2));
    ok(str2.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", str2.GetLength());

    str = _X("\t\t\t\t\t");
    str2 = str.Trim();
    ok(str2 == _X(""), "Expected str2 to be '', was: %s\n", dbgstrx(str2));
    ok(str2.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", str2.GetLength());
}

TEST_NAMEX(env)
{
    CStringX test;
    BOOL ret;
    ok(test.IsEmpty() == true, "Expected test to be empty\n");
    ok(test.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", test.GetLength());
    ok(test.GetAllocLength() == 0, "Expected GetAllocLength() to be 0, was: %i\n", test.GetAllocLength());

    XCHAR buf[512];
    GetWindowsDirectoryX(buf, _countof(buf));

    ret = test.GetEnvironmentVariable(_X("SystemDrive"));
    ok(!!ret, "Expected %%SystemDrive%% to exist\n");
    ok(test.IsEmpty() == false, "Expected test to have a valid result\n");
    ok(test.GetLength() == 2, "Expected GetLength() to be 2, was: %i\n", test.GetLength());
    if (test.GetLength() == 2)
    {
        ok(test[0] == buf[0], "Expected test[0] to equal buf[0], was: %c, %c\n", test[0], buf[0]);
        ok(test[1] == buf[1], "Expected test[1] to equal buf[1], was: %c, %c\n", test[1], buf[1]);
    }

    ret = test.GetEnvironmentVariable(_X("SystemRoot"));
    ok(!!ret, "Expected %%SystemRoot%% to exist\n");
    ok(test.IsEmpty() == false, "Expected test to have a valid result\n");
    ok(test == buf, "Expected test to be %s, was %s\n", dbgstrx(buf), dbgstrx(test));

    ret = test.GetEnvironmentVariable(_X("some non existing env var"));
    ok(!ret, "Expected %%some non existing env var%% to not exist\n");
    ok(test.IsEmpty() == true, "Expected test to be empty\n");
    ok(test.GetLength() == 0, "Expected GetLength() to be 0, was: %i\n", test.GetLength());
}

TEST_NAMEX(load_str)
{
    CStringX str;

    ok(str.LoadString(0) == FALSE, "LoadString should fail.\n");

    ok(str.LoadString(IDS_TEST1) == TRUE, "LoadString failed.\n");
    ok(str == _X("Test string one."), "The value was '%s'\n", dbgstrx(str));

    ok(str.LoadString(IDS_TEST2) == TRUE, "LoadString failed.\n");
    ok(str == _X("I am a happy BSTR"), "The value was '%s'\n", dbgstrx(str));

    ok(str.LoadString(0) == FALSE, "LoadString should fail.\n");
    ok(str == _X("I am a happy BSTR"), "The value was '%s'\n", dbgstrx(str));

    XCHAR *xNULL = NULL;
    CStringX str0(xNULL);
    ok(str0.IsEmpty(), "str0 should be empty.\n");

    YCHAR *yNULL = NULL;
    CStringX str1(yNULL);
    ok(str1.IsEmpty(), "str1 should be empty.\n");

    CStringX str2(MAKEINTRESOURCEX(IDS_TEST1));
    ok(str2 == _X("Test string one."), "The value was '%s'\n", dbgstrx(str2));

    CStringX str3(MAKEINTRESOURCEX(IDS_TEST2));
    ok(str3 == _X("I am a happy BSTR"), "The value was '%s'\n", dbgstrx(str3));

    CStringX str4(MAKEINTRESOURCEY(IDS_TEST1));
    ok(str4 == _X("Test string one."), "The value was '%s'\n", dbgstrx(str4));

    CStringX str5(MAKEINTRESOURCEY(IDS_TEST2));
    ok(str5 == _X("I am a happy BSTR"), "The value was '%s'\n", dbgstrx(str5));
}

TEST_NAMEX(bstr)
{
    CStringX str;

    str = _X("Some test text here...");

    BSTR bstr = str.AllocSysString();
    ok(!!bstr, "Expected a valid pointer\n");
    if (bstr)
    {
        ok(!wcscmp(bstr, L"Some test text here..."), "Expected 'Some test text here...', got: '%S'\n", bstr);
        ::SysFreeString(bstr);
    }
}
