1 stdcall -noname QueryThemeServices()
2 stdcall -noname OpenThemeFile(wstr wstr wstr ptr long)
3 stdcall -noname CloseThemeFile(ptr)
4 stdcall -noname ApplyTheme(ptr ptr ptr)
5 stdcall CloseThemeData(ptr)
6 stdcall DrawThemeBackground(ptr ptr long long ptr ptr)
7 stdcall -noname GetThemeDefaults(wstr wstr long wstr long)
8 stdcall -noname EnumThemes(wstr ptr ptr)
9 stdcall -noname EnumThemeColors(wstr wstr long ptr)
10 stdcall -noname EnumThemeSizes(wstr wstr long ptr)
11 stdcall -noname ParseThemeIniFile(wstr wstr ptr ptr)
12 stdcall DrawThemeEdge(ptr ptr long long ptr long long ptr)
13 stdcall -noname DrawNCPreview(ptr long ptr wstr wstr wstr ptr ptr)
14 stub -noname RegisterDefaultTheme
15 stub -noname DumpLoadedThemeToTextFile
16 stdcall -noname OpenThemeDataFromFile(ptr ptr wstr long)
17 stub -noname OpenThemeFileFromData
18 stub -noname GetThemeSysSize96
19 stub -noname GetThemeSysFont96
20 stub -noname SessionAllocate
21 stub -noname SessionFree
22 stub -noname ThemeHooksOn
23 stub -noname ThemeHooksOff
24 stub -noname AreThemeHooksActive
25 stub -noname GetCurrentChangeNumber
26 stub -noname GetNewChangeNumber
27 stub -noname SetGlobalTheme
28 stub -noname GetGlobalTheme
29 stdcall -noname CheckThemeSignature(wstr)
30 stub -noname LoadTheme
31 stub -noname InitUserTheme
32 stub -noname InitUserRegistry
33 stub -noname ReestablishServerConnection
34 stdcall -noname ThemeHooksInstall()
35 stdcall -noname ThemeHooksRemove()
36 stub -noname RefreshThemeForTS
37 stdcall DrawThemeIcon(ptr ptr long long ptr ptr long)
38 stdcall DrawThemeParentBackground(ptr ptr ptr)
39 stdcall DrawThemeText(ptr ptr long long wstr long long long ptr)
40 stdcall EnableThemeDialogTexture(ptr long)
41 stdcall EnableTheming(long)
42 stdcall GetCurrentThemeName(wstr long wstr long wstr long)
43 stdcall -noname ClassicGetSystemMetrics(long)
44 stdcall -noname ClassicSystemParametersInfoA(long long ptr long)
45 stdcall -noname ClassicSystemParametersInfoW(long long ptr long)
46 stdcall -noname ClassicAdjustWindowRectEx(ptr long long long)
47 stdcall DrawThemeBackgroundEx(ptr ptr long long ptr ptr)
48 stdcall -noname GetThemeParseErrorInfo(ptr)
49 stdcall GetThemeAppProperties()
50 stdcall GetThemeBackgroundContentRect(ptr ptr long long ptr ptr)
51 stdcall GetThemeBackgroundExtent(ptr ptr long long ptr ptr)
52 stdcall GetThemeBackgroundRegion(ptr ptr long long ptr ptr)
53 stdcall GetThemeBool(ptr long long long ptr)
54 stdcall GetThemeColor(ptr long long long ptr)
55 stdcall GetThemeDocumentationProperty(wstr wstr wstr long)
56 stdcall GetThemeEnumValue(ptr long long long ptr)
57 stdcall GetThemeFilename(ptr long long long wstr long)
58 stdcall GetThemeFont(ptr ptr long long long ptr)
59 stdcall GetThemeInt(ptr long long long ptr)
60 stub -noname CreateThemeDataFromObjects
61 stdcall -noname OpenThemeDataEx(ptr wstr long)
62 stub -noname ServerClearStockObjects
63 stub -noname MarkSelection
#64 ProcessLoadTheme_RunDLLW
#65 SetSystemVisualStyle
#66 ServiceClearStockObjects
67 stdcall GetThemeIntList(ptr long long long ptr)
68 stdcall GetThemeMargins(ptr ptr long long long ptr ptr)
69 stdcall GetThemeMetric(ptr ptr long long long ptr)
70 stdcall GetThemePartSize(ptr ptr long long ptr long ptr)
71 stdcall GetThemePosition(ptr long long long ptr)
72 stdcall GetThemePropertyOrigin(ptr long long long ptr)
#73 IsThemeActiveByPolicy
74 stdcall GetThemeRect(ptr long long long ptr)
75 stdcall GetThemeString(ptr long long long wstr long)
76 stdcall GetThemeSysBool(ptr long)
77 stdcall GetThemeSysColor(ptr long)
78 stdcall GetThemeSysColorBrush(ptr long)
79 stdcall GetThemeSysFont(ptr long ptr)
80 stdcall GetThemeSysInt(ptr long ptr)
81 stdcall GetThemeSysSize(ptr long)
82 stdcall GetThemeSysString(ptr long wstr long)
83 stdcall GetThemeTextExtent(ptr ptr long long wstr long long ptr ptr)
84 stdcall GetThemeTextMetrics(ptr ptr long long ptr)
85 stdcall GetWindowTheme(ptr)
86 stdcall HitTestThemeBackground(ptr long long long long ptr long double ptr)
87 stdcall IsAppThemed()
88 stdcall IsThemeActive()
89 stdcall IsThemeBackgroundPartiallyTransparent(ptr long long)
90 stdcall IsThemeDialogTextureEnabled(ptr)
91 stdcall IsThemePartDefined(ptr long long)
92 stdcall OpenThemeData(ptr wstr)
93 stdcall SetThemeAppProperties(long)
94 stdcall SetWindowTheme(ptr wstr wstr)
95 stdcall ThemeInitApiHook(long ptr)
