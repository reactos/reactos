1 stdcall -private DllCanUnloadNow() MSI_DllCanUnloadNow
2 stdcall -private DllGetClassObject(ptr ptr ptr) MSI_DllGetClassObject
3 stdcall -private DllRegisterServer() MSI_DllRegisterServer
4 stdcall -private DllUnregisterServer() MSI_DllUnregisterServer
5 stdcall MsiAdvertiseProductA(str str str long)
6 stdcall MsiAdvertiseProductW(wstr wstr wstr long)
7 stdcall MsiCloseAllHandles()
8 stdcall MsiCloseHandle(long)
9 stub MsiCollectUserInfoA
10 stub MsiCollectUserInfoW
11 stub MsiConfigureFeatureA
12 stub MsiConfigureFeatureFromDescriptorA
13 stub MsiConfigureFeatureFromDescriptorW
14 stub MsiConfigureFeatureW
15 stdcall MsiConfigureProductA(str long long)
16 stdcall MsiConfigureProductW(wstr long long)
17 stdcall MsiCreateRecord(long)
18 stdcall MsiDatabaseApplyTransformA(long str long)
19 stdcall MsiDatabaseApplyTransformW(long wstr long)
20 stdcall MsiDatabaseCommit(long)
21 stub MsiDatabaseExportA
22 stub MsiDatabaseExportW
23 stdcall MsiDatabaseGenerateTransformA(long long str long long)
24 stdcall MsiDatabaseGenerateTransformW(long long wstr long long)
25 stdcall MsiDatabaseGetPrimaryKeysA(long str ptr)
26 stdcall MsiDatabaseGetPrimaryKeysW(long wstr ptr)
27 stdcall MsiDatabaseImportA(str str)
28 stdcall MsiDatabaseImportW(wstr wstr)
29 stub MsiDatabaseMergeA
30 stub MsiDatabaseMergeW
31 stdcall MsiDatabaseOpenViewA(long str ptr)
32 stdcall MsiDatabaseOpenViewW(long wstr ptr)
33 stdcall MsiDoActionA(long str)
34 stdcall MsiDoActionW(long wstr)
35 stub MsiEnableUIPreview
36 stdcall MsiEnumClientsA(str long ptr)
37 stdcall MsiEnumClientsW(wstr long ptr)
38 stdcall MsiEnumComponentQualifiersA(str long str ptr str ptr)
39 stdcall MsiEnumComponentQualifiersW(wstr long wstr ptr wstr ptr)
40 stdcall MsiEnumComponentsA(long ptr)
41 stdcall MsiEnumComponentsW(long ptr)
42 stdcall MsiEnumFeaturesA(str long ptr ptr)
43 stdcall MsiEnumFeaturesW(wstr long ptr ptr)
44 stdcall MsiEnumProductsA(long ptr)
45 stdcall MsiEnumProductsW(long ptr)
46 stdcall MsiEvaluateConditionA(long str)
47 stdcall MsiEvaluateConditionW(long wstr)
48 stub MsiGetLastErrorRecord
49 stdcall MsiGetActiveDatabase(long)
50 stdcall MsiGetComponentStateA(long str ptr ptr)
51 stdcall MsiGetComponentStateW(long wstr ptr ptr)
52 stub MsiGetDatabaseState
53 stub MsiGetFeatureCostA
54 stub MsiGetFeatureCostW
55 stub MsiGetFeatureInfoA
56 stub MsiGetFeatureInfoW
57 stdcall MsiGetFeatureStateA(long str ptr ptr)
58 stdcall MsiGetFeatureStateW(long wstr ptr ptr)
59 stub MsiGetFeatureUsageA
60 stub MsiGetFeatureUsageW
61 stub MsiGetFeatureValidStatesA
62 stub MsiGetFeatureValidStatesW
63 stub MsiGetLanguage
64 stdcall MsiGetMode(long long)
65 stdcall MsiGetProductCodeA(str str)
66 stdcall MsiGetProductCodeW(wstr wstr)
67 stdcall MsiGetProductInfoA(str str str long)
68 stub MsiGetProductInfoFromScriptA
69 stub MsiGetProductInfoFromScriptW
70 stdcall MsiGetProductInfoW(wstr wstr wstr long)
71 stdcall MsiGetProductPropertyA(long str ptr ptr)
72 stdcall MsiGetProductPropertyW(long wstr ptr ptr)
73 stdcall MsiGetPropertyA(ptr str str ptr)
74 stdcall MsiGetPropertyW(ptr wstr wstr ptr)
75 stdcall MsiGetSourcePathA(long str ptr ptr)
76 stdcall MsiGetSourcePathW(long wstr ptr ptr)
77 stdcall MsiGetSummaryInformationA(long str long ptr)
78 stdcall MsiGetSummaryInformationW(long wstr long ptr)
79 stdcall MsiGetTargetPathA(long str ptr ptr)
80 stdcall MsiGetTargetPathW(long wstr ptr ptr)
81 stub MsiGetUserInfoA
82 stub MsiGetUserInfoW
83 stub MsiInstallMissingComponentA
84 stub MsiInstallMissingComponentW
85 stub MsiInstallMissingFileA
86 stub MsiInstallMissingFileW
87 stdcall MsiInstallProductA(str str)
88 stdcall MsiInstallProductW(wstr wstr)
89 stdcall MsiLocateComponentA(str ptr long)
90 stdcall MsiLocateComponentW(wstr ptr long)
91 stdcall MsiOpenDatabaseA(str str ptr)
92 stdcall MsiOpenDatabaseW(wstr wstr ptr)
93 stdcall MsiOpenPackageA(str ptr)
94 stdcall MsiOpenPackageW(wstr ptr)
95 stdcall MsiOpenProductA(str ptr)
96 stdcall MsiOpenProductW(wstr ptr)
97 stub MsiPreviewBillboardA
98 stub MsiPreviewBillboardW
99 stub MsiPreviewDialogA
100 stub MsiPreviewDialogW
101 stub MsiProcessAdvertiseScriptA
102 stub MsiProcessAdvertiseScriptW
103 stdcall MsiProcessMessage(long long long)
104 stub MsiProvideComponentA
105 stdcall MsiProvideComponentFromDescriptorA(str ptr ptr ptr)
106 stdcall MsiProvideComponentFromDescriptorW(wstr ptr ptr ptr)
107 stub MsiProvideComponentW
108 stub MsiProvideQualifiedComponentA
109 stub MsiProvideQualifiedComponentW
110 stdcall MsiQueryFeatureStateA(str str)
111 stdcall MsiQueryFeatureStateW(wstr wstr)
112 stdcall MsiQueryProductStateA(str)
113 stdcall MsiQueryProductStateW(wstr)
114 stdcall MsiRecordDataSize(long long)
115 stdcall MsiRecordGetFieldCount(long)
116 stdcall MsiRecordGetInteger(long long)
117 stdcall MsiRecordGetStringA(long long ptr ptr)
118 stdcall MsiRecordGetStringW(long long ptr ptr)
119 stdcall MsiRecordIsNull(long long)
120 stdcall MsiRecordReadStream(long long ptr ptr)
121 stdcall MsiRecordSetInteger(long long long)
122 stdcall MsiRecordSetStreamA(long long str)
123 stdcall MsiRecordSetStreamW(long long wstr)
124 stdcall MsiRecordSetStringA(long long str)
125 stdcall MsiRecordSetStringW(long long wstr)
126 stub MsiReinstallFeatureA
127 stub MsiReinstallFeatureFromDescriptorA
128 stub MsiReinstallFeatureFromDescriptorW
129 stub MsiReinstallFeatureW
130 stdcall MsiReinstallProductA(str long)
131 stdcall MsiReinstallProductW(wstr long)
132 stub MsiSequenceA
133 stub MsiSequenceW
134 stub MsiSetComponentStateA
135 stub MsiSetComponentStateW
136 stdcall MsiSetExternalUIA(ptr long ptr)
137 stub MsiSetExternalUIW
138 stdcall MsiSetFeatureStateA(long str long)
139 stdcall MsiSetFeatureStateW(long wstr long)
140 stub MsiSetInstallLevel
141 stdcall MsiSetInternalUI(long ptr)
142 stub MsiVerifyDiskSpace
143 stub MsiSetMode
144 stdcall MsiSetPropertyA(long str str)
145 stdcall MsiSetPropertyW(long wstr wstr)
146 stdcall MsiSetTargetPathA(long str str)
147 stdcall MsiSetTargetPathW(long wstr wstr)
148 stdcall MsiSummaryInfoGetPropertyA(long long ptr ptr ptr ptr ptr)
149 stdcall MsiSummaryInfoGetPropertyCount(long ptr)
150 stdcall MsiSummaryInfoGetPropertyW(long long ptr ptr ptr ptr ptr)
151 stub MsiSummaryInfoPersist
152 stub MsiSummaryInfoSetPropertyA
153 stub MsiSummaryInfoSetPropertyW
154 stub MsiUseFeatureA
155 stub MsiUseFeatureW
156 stdcall MsiVerifyPackageA(str)
157 stdcall MsiVerifyPackageW(wstr)
158 stdcall MsiViewClose(long)
159 stdcall MsiViewExecute(long long)
160 stdcall MsiViewFetch(long ptr)
161 stub MsiViewGetErrorA
162 stub MsiViewGetErrorW
163 stdcall MsiViewModify(long long long)
164 stdcall MsiDatabaseIsTablePersistentA(long str)
165 stdcall MsiDatabaseIsTablePersistentW(long wstr)
166 stdcall MsiViewGetColumnInfo(long long ptr)
167 stdcall MsiRecordClearData(long)
168 stdcall MsiEnableLogA(long str long)
169 stdcall MsiEnableLogW(long wstr long)
170 stdcall MsiFormatRecordA(long long ptr ptr)
171 stdcall MsiFormatRecordW(long long ptr ptr)
172 stdcall MsiGetComponentPathA(str str ptr ptr)
173 stdcall MsiGetComponentPathW(wstr wstr ptr ptr)
174 stdcall MsiApplyPatchA(str str long str)
175 stdcall MsiApplyPatchW(wstr wstr long wstr)
176 stub MsiAdvertiseScriptA
177 stub MsiAdvertiseScriptW
178 stub MsiGetPatchInfoA
179 stub MsiGetPatchInfoW
180 stub MsiEnumPatchesA
181 stub MsiEnumPatchesW
182 stdcall DllGetVersion(ptr) MSI_DllGetVersion
183 stub MsiGetProductCodeFromPackageCodeA
184 stub MsiGetProductCodeFromPackageCodeW
185 stub MsiCreateTransformSummaryInfoA
186 stub MsiCreateTransformSummaryInfoW
187 stub MsiQueryFeatureStateFromDescriptorA
188 stub MsiQueryFeatureStateFromDescriptorW
189 stub MsiConfigureProductExA
190 stub MsiConfigureProductExW
191 stub MsiInvalidateFeatureCache
192 stub MsiUseFeatureExA
193 stub MsiUseFeatureExW
194 stdcall MsiGetFileVersionA(str str ptr str ptr)
195 stdcall MsiGetFileVersionW(wstr wstr ptr wstr ptr)
196 stdcall MsiLoadStringA(long long long long long)
197 stdcall MsiLoadStringW(long long long long long)
198 stdcall MsiMessageBoxA(long long long long long long)
199 stdcall MsiMessageBoxW(long long long long long long)
200 stub MsiDecomposeDescriptorA
201 stub MsiDecomposeDescriptorW
202 stub MsiProvideQualifiedComponentExA
203 stub MsiProvideQualifiedComponentExW
204 stdcall MsiEnumRelatedProductsA(str long long str)
205 stub MsiEnumRelatedProductsW
206 stub MsiSetFeatureAttributesA
207 stub MsiSetFeatureAttributesW
208 stub MsiSourceListClearAllA
209 stub MsiSourceListClearAllW
210 stub MsiSourceListAddSourceA
211 stub MsiSourceListAddSourceW
212 stub MsiSourceListForceResolutionA
213 stub MsiSourceListForceResolutionW
214 stub MsiIsProductElevatedA
215 stub MsiIsProductElevatedW
216 stub MsiGetShortcutTargetA
217 stub MsiGetShortcutTargetW
218 stub MsiGetFileHashA
219 stub MsiGetFileHashW
220 stub MsiEnumComponentCostsA
221 stub MsiEnumComponentCostsW
222 stub MsiCreateAndVerifyInstallerDirectory
223 stdcall MsiGetFileSignatureInformationA(str long ptr ptr ptr)
224 stdcall MsiGetFileSignatureInformationW(wstr long ptr ptr ptr)
225 stdcall MsiProvideAssemblyA(str str long long str ptr)
226 stdcall MsiProvideAssemblyW(wstr wstr long long wstr ptr)
227 stdcall MsiAdvertiseProductExA(str str str long long long)
228 stdcall MsiAdvertiseProductExW(wstr wstr wstr long long long)
229 stub MsiNotifySidChangeA
230 stub MsiNotifySidChangeW
231 stdcall MsiOpenPackageExA(str long ptr)
232 stdcall MsiOpenPackageExW(wstr long ptr)
233 stub MsiDeleteUserDataA
234 stub MsiDeleteUserDataW
235 stub Migrate10CachedPackagesA
236 stub Migrate10CachedPackagesW
