# Undocumented functions - Names derived from debug symbols
1  stdcall -noname QueryThemeServices()
2  stdcall -noname OpenThemeFile(wstr wstr wstr ptr long)
3  stdcall -noname CloseThemeFile(ptr)
4  stdcall -noname ApplyTheme(ptr ptr ptr)
7  stdcall -noname GetThemeDefaults(wstr wstr long wstr long)
8  stdcall -noname EnumThemes(wstr ptr ptr)
9  stdcall -noname EnumThemeColors(wstr wstr long ptr)
10 stdcall -noname EnumThemeSizes(wstr wstr long ptr)
11 stdcall -noname ParseThemeIniFile(wstr wstr ptr ptr)
13 stub -noname DrawNCPreview
14 stub -noname RegisterDefaultTheme
15 stub -noname DumpLoadedThemeToTextFile
16 stub -noname OpenThemeDataFromFile
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
34 stub -noname ThemeHooksInstall
35 stub -noname ThemeHooksRemove
36 stub -noname RefreshThemeForTS
43 stub -noname ClassicGetSystemMetrics
44 stub -noname ClassicSystemParametersInfoA
45 stub -noname ClassicSystemParametersInfoW
46 stub -noname ClassicAdjustWindowRectEx
48 stub -noname GetThemeParseErrorInfo
60 stub -noname CreateThemeDataFromObjects
61 stub -noname OpenThemeDataEx
62 stub -noname ServerClearStockObjects
63 stub -noname MarkSelection

# Standard functions
@ stdcall CloseThemeData(ptr)
@ stdcall DrawThemeBackground(ptr ptr long long ptr ptr)
@ stdcall DrawThemeBackgroundEx(ptr ptr long long ptr ptr)
@ stdcall DrawThemeEdge(ptr ptr long long ptr long long ptr)
@ stdcall DrawThemeIcon(ptr ptr long long ptr ptr long)
@ stdcall DrawThemeParentBackground(ptr ptr ptr)
@ stdcall DrawThemeText(ptr ptr long long wstr long long long ptr)
@ stdcall EnableThemeDialogTexture(ptr long)
@ stdcall EnableTheming(long)
@ stdcall GetCurrentThemeName(wstr long wstr long wstr long)
@ stdcall GetThemeAppProperties()
@ stdcall GetThemeBackgroundContentRect(ptr ptr long long ptr ptr)
@ stdcall GetThemeBackgroundExtent(ptr ptr long long ptr ptr)
@ stdcall GetThemeBackgroundRegion(ptr ptr long long ptr ptr)
@ stdcall GetThemeBool(ptr long long long ptr)
@ stdcall GetThemeColor(ptr long long long ptr)
@ stdcall GetThemeDocumentationProperty(wstr wstr wstr long)
@ stdcall GetThemeEnumValue(ptr long long long ptr)
@ stdcall GetThemeFilename(ptr long long long wstr long)
@ stdcall GetThemeFont(ptr ptr long long long ptr)
@ stdcall GetThemeInt(ptr long long long ptr)
@ stdcall GetThemeIntList(ptr long long long ptr)
@ stdcall GetThemeMargins(ptr ptr long long long ptr ptr)
@ stdcall GetThemeMetric(ptr ptr long long long ptr)
@ stdcall GetThemePartSize(ptr ptr long long ptr long ptr)
@ stdcall GetThemePosition(ptr long long long ptr)
@ stdcall GetThemePropertyOrigin(ptr long long long ptr)
@ stdcall GetThemeRect(ptr long long long ptr)
@ stdcall GetThemeString(ptr long long long wstr long)
@ stdcall GetThemeSysBool(ptr long)
@ stdcall GetThemeSysColor(ptr long)
@ stdcall GetThemeSysColorBrush(ptr long)
@ stdcall GetThemeSysFont(ptr long ptr)
@ stdcall GetThemeSysInt(ptr long ptr)
@ stdcall GetThemeSysSize(ptr long)
@ stdcall GetThemeSysString(ptr long wstr long)
@ stdcall GetThemeTextExtent(ptr ptr long long wstr long long ptr ptr)
@ stdcall GetThemeTextMetrics(ptr ptr long long ptr)
@ stdcall GetWindowTheme(ptr)
@ stdcall HitTestThemeBackground(ptr long long long long ptr long double ptr)
@ stdcall IsAppThemed()
@ stdcall IsThemeActive()
@ stdcall IsThemeBackgroundPartiallyTransparent(ptr long long)
@ stdcall IsThemeDialogTextureEnabled(ptr)
@ stdcall IsThemePartDefined(ptr long long)
@ stdcall OpenThemeData(ptr wstr)
@ stdcall SetThemeAppProperties(long)
@ stdcall SetWindowTheme(ptr wstr wstr)
