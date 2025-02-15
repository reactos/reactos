1 stdcall -noname QueryThemeServices()
2 stdcall -noname OpenThemeFile(wstr wstr wstr ptr long)
3 stdcall -noname CloseThemeFile(ptr)
4 stdcall -noname ApplyTheme(ptr ptr ptr)
5 stdcall BeginBufferedAnimation(ptr ptr ptr long ptr ptr ptr ptr)
6 stdcall BeginBufferedPaint(ptr ptr long ptr ptr)
7 stdcall -noname GetThemeDefaults(wstr wstr long wstr long)
8 stdcall -noname EnumThemes(wstr ptr ptr)
9 stdcall -noname EnumThemeColors(wstr wstr long ptr)
10 stdcall -noname EnumThemeSizes(wstr wstr long ptr)
11 stdcall -noname ParseThemeIniFile(wstr wstr ptr ptr)
12 stdcall BufferedPaintClear(ptr ptr)
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
37 stdcall BufferedPaintInit()
38 stdcall BufferedPaintRenderAnimation(ptr ptr)
39 stdcall BufferedPaintSetAlpha(ptr ptr long)
40 stdcall BufferedPaintStopAllAnimations(ptr)
41 stdcall BufferedPaintUnInit()
42 stdcall CloseThemeData(ptr)
43 stdcall -noname ClassicGetSystemMetrics(long)
44 stdcall -noname ClassicSystemParametersInfoA(long long ptr long)
45 stdcall -noname ClassicSystemParametersInfoW(long long ptr long)
46 stdcall -noname ClassicAdjustWindowRectEx(ptr long long long)
47 stdcall DrawThemeBackgroundEx(ptr ptr long long ptr ptr)
48 stub -noname GetThemeParseErrorInfo
49 stdcall -stub OpenNcThemeData()
50 stdcall -stub IsThemeClassDefined(ptr ptr long long ptr ptr)
51 stdcall DrawThemeBackground(ptr ptr long long ptr ptr)
52 stdcall DrawThemeEdge(ptr ptr long long ptr long long ptr)
53 stdcall DrawThemeIcon(ptr ptr long long ptr ptr long)
54 stdcall DrawThemeParentBackground(ptr ptr ptr)
55 stub DrawThemeParentBackgroundEx
56 stdcall DrawThemeText(ptr ptr long long wstr long long long ptr)
57 stdcall DrawThemeTextEx(ptr ptr long long wstr long long ptr ptr)
58 stdcall EnableThemeDialogTexture(ptr long)
59 stdcall EnableTheming(long)
60 stub -noname CreateThemeDataFromObjects
61 stdcall -noname OpenThemeDataEx(ptr wstr long)
62 stub -noname ServerClearStockObjects
63 stub -noname MarkSelection
64 stub ProcessLoadTheme_RunDLLW
65 stub SetSystemVisualStyle
66 stub ServiceClearStockObjects
67 stdcall -stub AddThemeAppCompatFlag(ptr long long long ptr)
68 stdcall -stub ResetThemeAppCompatFlags(ptr ptr long long long ptr ptr)
69 stdcall -stub EnumThemeProperties(ptr ptr long long long ptr)
70 stdcall EndBufferedAnimation(ptr long)
71 stdcall EndBufferedPaint(ptr long)
72 stdcall -stub DrawThemeIconEx(ptr long long long ptr)
73 stub IsThemeActiveByPolicy
74 stdcall -stub GetThemeClass(ptr long long long ptr)
75 stdcall -stub ThemeForwardCapturedMouseMessage(ptr long long long wstr long)
76 stdcall -stub EnableServiceConnection(ptr long)
77 stdcall -stub Remote_LoadTheme(ptr long) ; This exits a thread
78 stdcall GetBufferedPaintBits(ptr ptr ptr)
79 stdcall GetBufferedPaintDC(ptr)
80 stdcall GetBufferedPaintTargetDC(ptr)
81 stdcall GetBufferedPaintTargetRect(ptr ptr)
82 stdcall GetCurrentThemeName(wstr long wstr long wstr long)
83 stdcall GetThemeAppProperties()
84 stdcall GetThemeBackgroundContentRect(ptr ptr long long ptr ptr)
85 stdcall GetThemeBackgroundExtent(ptr ptr long long ptr ptr)
86 stdcall GetThemeBackgroundRegion(ptr ptr long long ptr ptr)
87 stub GetThemeBitMap
88 stdcall GetThemeBool(ptr long long long ptr)
89 stdcall GetThemeColor(ptr long long long ptr)
90 stdcall GetThemeDocumentationProperty(wstr wstr wstr long)
91 stdcall GetThemeEnumValue(ptr long long long ptr)
92 stdcall GetThemeFilename(ptr long long long wstr long)
93 stdcall GetThemeFont(ptr ptr long long long ptr)
94 stdcall GetThemeInt(ptr long long long ptr)
95 stdcall GetThemeIntList(ptr long long long ptr)
96 stdcall GetThemeMargins(ptr ptr long long long ptr ptr)
97 stdcall GetThemeMetric(ptr ptr long long long ptr)
98 stdcall GetThemePartSize(ptr ptr long long ptr long ptr)
99 stdcall GetThemePosition(ptr long long long ptr)
100 stdcall GetThemePropertyOrigin(ptr long long long ptr)
101 stdcall GetThemeRect(ptr long long long ptr)
102 stub GetThemeStream
103 stdcall GetThemeString(ptr long long long wstr long)
104 stdcall GetThemeSysBool(ptr long)
105 stdcall GetThemeSysColor(ptr long)
106 stdcall GetThemeSysColorBrush(ptr long)
107 stdcall GetThemeSysFont(ptr long ptr)
108 stdcall GetThemeSysInt(ptr long ptr)
109 stdcall GetThemeSysSize(ptr long)
110 stdcall GetThemeSysString(ptr long wstr long)
111 stdcall GetThemeTextExtent(ptr ptr long long wstr long long ptr ptr)
112 stdcall GetThemeTextMetrics(ptr ptr long long ptr)
113 stdcall GetThemeTransitionDuration(ptr long long long long ptr)
114 stdcall GetWindowTheme(ptr)
115 stdcall HitTestThemeBackground(ptr long long long long ptr long double ptr)
116 stdcall IsAppThemed()
117 stdcall IsCompositionActive()
118 stdcall IsThemeActive()
119 stdcall IsThemeBackgroundPartiallyTransparent(ptr long long)
120 stdcall IsThemeDialogTextureEnabled(ptr)
121 stdcall IsThemePartDefined(ptr long long)
122 stdcall OpenThemeData(ptr wstr)
123 stdcall SetThemeAppProperties(long)
124 stdcall SetWindowTheme(ptr wstr wstr)
125 stdcall SetWindowThemeAttribute(ptr long ptr long)
126 stdcall ThemeInitApiHook(long ptr)

@ stdcall OpenThemeDataForDpi(ptr wstr long) ;win7
