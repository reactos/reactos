  8 stub @
 10 stdcall MAPILogonEx(long ptr ptr long ptr)
 11 stdcall MAPILogonEx@20(long ptr ptr long ptr) MAPILogonEx
 12 stdcall MAPIAllocateBuffer(long ptr)
 13 stdcall MAPIAllocateBuffer@8(long ptr) MAPIAllocateBuffer
 14 stdcall MAPIAllocateMore(long ptr ptr)
 15 stdcall MAPIAllocateMore@12(long ptr ptr) MAPIAllocateMore
 16 stdcall MAPIFreeBuffer(ptr)
 17 stdcall MAPIFreeBuffer@4(ptr) MAPIFreeBuffer
 18 stdcall MAPIAdminProfiles(long ptr)
 19 stdcall MAPIAdminProfiles@8(long ptr) MAPIAdminProfiles
 20 stdcall MAPIInitialize(ptr)
 21 stdcall MAPIInitialize@4(ptr) MAPIInitialize
 22 stdcall MAPIUninitialize()
 23 stdcall MAPIUninitialize@0() MAPIUninitialize
 24 stub PRProviderInit
 25 stub LAUNCHWIZARD
 26 stub LaunchWizard@20
 27 stub DllGetClassObject
 28 stdcall -private DllCanUnloadNow()
 29 stub MAPIOpenFormMgr
 30 stub MAPIOpenFormMgr@8
 31 stdcall MAPIOpenLocalFormContainer(ptr)
 32 stdcall MAPIOpenLocalFormContainer@4(ptr) MAPIOpenLocalFormContainer
 33 stdcall ScInitMapiUtil@4(long) ScInitMapiUtil
 34 stdcall DeinitMapiUtil@0() DeinitMapiUtil
 35 stub ScGenerateMuid@4
 36 stub HrAllocAdviseSink@12
 41 stdcall WrapProgress@20(ptr ptr ptr ptr ptr) WrapProgress
 42 stdcall HrThisThreadAdviseSink@8(ptr ptr) HrThisThreadAdviseSink
 43 stub ScBinFromHexBounded@12
 44 stdcall FBinFromHex@8(ptr ptr) FBinFromHex
 45 stdcall HexFromBin@12(ptr long ptr) HexFromBin
 46 stub BuildDisplayTable@40
 47 stdcall SwapPlong@8(ptr long) SwapPlong
 48 stdcall SwapPword@8(ptr long) SwapPword
 49 stub MAPIInitIdle@4
 50 stub MAPIDeinitIdle@0
 51 stub InstallFilterHook@4
 52 stub FtgRegisterIdleRoutine@20
 53 stub EnableIdleRoutine@8
 54 stub DeregisterIdleRoutine@4
 55 stub ChangeIdleRoutine@28
 59 stdcall MAPIGetDefaultMalloc@0() MAPIGetDefaultMalloc
 60 stdcall CreateIProp@24(ptr ptr ptr ptr ptr ptr) CreateIProp
 61 stub CreateTable@36
 62 stdcall MNLS_lstrlenW@4(wstr) MNLS_lstrlenW
 63 stdcall MNLS_lstrcmpW@8(wstr wstr) MNLS_lstrcmpW
 64 stdcall MNLS_lstrcpyW@8(ptr wstr) MNLS_lstrcpyW
 65 stdcall MNLS_CompareStringW@24(long wstr wstr) MNLS_CompareStringW
 66 stdcall MNLS_MultiByteToWideChar@24(long long str long ptr long) kernel32.MultiByteToWideChar
 67 stdcall MNLS_WideCharToMultiByte@32(long long wstr long ptr long ptr ptr) kernel32.WideCharToMultiByte
 68 stdcall MNLS_IsBadStringPtrW@8(ptr long) kernel32.IsBadStringPtrW
 72 stdcall FEqualNames@8(ptr ptr) FEqualNames
 73 stub WrapStoreEntryID@24
 74 stdcall IsBadBoundedStringPtr@8(ptr long) IsBadBoundedStringPtr
 75 stub HrQueryAllRows@24
 76 stdcall PropCopyMore@16(ptr ptr ptr ptr) PropCopyMore
 77 stdcall UlPropSize@4(ptr) UlPropSize
 78 stdcall FPropContainsProp@12(ptr ptr long) FPropContainsProp
 79 stdcall FPropCompareProp@12(ptr long ptr) FPropCompareProp
 80 stdcall LPropCompareProp@8(ptr ptr) LPropCompareProp
 81 stub HrAddColumns@16
 82 stub HrAddColumnsEx@20
121 stdcall -ret64 FtAddFt@16(double double) MAPI32_FtAddFt
122 stub FtAdcFt@20
123 stdcall -ret64 FtSubFt@16(double double) MAPI32_FtSubFt
124 stdcall -ret64 FtMulDw@12(long double) MAPI32_FtMulDw
125 stdcall -ret64 FtMulDwDw@8(long long) MAPI32_FtMulDwDw
126 stdcall -ret64 FtNegFt@8(double) MAPI32_FtNegFt
127 stub FtDivFtBogus@20
128 stdcall UlAddRef@4(ptr) UlAddRef
129 stdcall UlRelease@4(ptr) UlRelease
130 stdcall SzFindCh@8(str long) shlwapi.StrChrA
131 stdcall SzFindLastCh@8(str str long) shlwapi.StrRChrA
132 stdcall SzFindSz@8(str str) shlwapi.StrStrA
133 stdcall UFromSz@4(str) UFromSz
135 stdcall HrGetOneProp@12(ptr long ptr) HrGetOneProp
136 stdcall HrSetOneProp@8(ptr ptr) HrSetOneProp
137 stdcall FPropExists@8(ptr long) FPropExists
138 stdcall PpropFindProp@12(ptr long long) PpropFindProp
139 stdcall FreePadrlist@4(ptr) FreePadrlist
140 stdcall FreeProws@4(ptr) FreeProws
141 stub HrSzFromEntryID@12
142 stub HrEntryIDFromSz@12
143 stub HrComposeEID@28
144 stub HrDecomposeEID@28
145 stub HrComposeMsgID@24
146 stub HrDecomposeMsgID@24
147 stdcall OpenStreamOnFile@24(ptr ptr ptr ptr ptr ptr) OpenStreamOnFile
148 stdcall OpenStreamOnFile(ptr ptr ptr ptr ptr ptr)
149 stub OpenTnefStream@28
150 stub OpenTnefStream
151 stub OpenTnefStreamEx@32
152 stub OpenTnefStreamEx
153 stub GetTnefStreamCodepage@12
154 stub GetTnefStreamCodepage
155 stdcall UlFromSzHex@4(ptr) UlFromSzHex
156 stub UNKOBJ_ScAllocate@12
157 stub UNKOBJ_ScAllocateMore@16
158 stub UNKOBJ_Free@8
159 stub UNKOBJ_FreeRows@8
160 stub UNKOBJ_ScCOAllocate@12
161 stub UNKOBJ_ScCOReallocate@12
162 stub UNKOBJ_COFree@8
163 stub UNKOBJ_ScSzFromIdsAlloc@20
164 stub ScCountNotifications@12
165 stub ScCopyNotifications@16
166 stub ScRelocNotifications@20
170 stdcall ScCountProps@12(long ptr ptr) ScCountProps
171 stdcall ScCopyProps@16(long ptr ptr ptr) ScCopyProps
172 stdcall ScRelocProps@20(long ptr ptr ptr ptr) ScRelocProps
173 stdcall LpValFindProp@12(long long ptr) LpValFindProp
174 stdcall ScDupPropset@16(long ptr ptr ptr) ScDupPropset
175 stdcall FBadRglpszA@8(ptr long) FBadRglpszA
176 stdcall FBadRglpszW@8(ptr long) FBadRglpszW
177 stdcall FBadRowSet@4(ptr) FBadRowSet
178 stub FBadRglpNameID@8
179 stdcall FBadPropTag@4(long) FBadPropTag
180 stdcall FBadRow@4(ptr) FBadRow
181 stdcall FBadProp@4(ptr) FBadProp
182 stdcall FBadColumnSet@4(ptr) FBadColumnSet
183 stub RTFSync@12
184 stub RTFSync
185 stub WrapCompressedRTFStream@12
186 stub WrapCompressedRTFStream
187 stub __ValidateParameters@8
188 stub __CPPValidateParameters@8
189 stub FBadSortOrderSet@4
190 stdcall FBadEntryList@4(ptr) FBadEntryList
191 stub FBadRestriction@4
192 stub ScUNCFromLocalPath@12
193 stub ScLocalPathFromUNC@12
194 stub HrIStorageFromStream@16
195 stub HrValidateIPMSubtree@20
196 stub OpenIMsgSession@12
197 stub CloseIMsgSession@4
198 stub OpenIMsgOnIStg@44
199 stub SetAttribIMsgOnIStg@16
200 stub GetAttribIMsgOnIStg@12
201 stub MapStorageSCode@4
202 stub ScMAPIXFromCMC
203 stub ScMAPIXFromSMAPI
204 stub EncodeID@12
205 stub FDecodeID@12
206 stub CchOfEncoding@4
207 stdcall CbOfEncoded@4(ptr) CbOfEncoded
208 stub MAPISendDocuments
209 stdcall MAPILogon(long ptr ptr long long ptr)
210 stdcall MAPILogoff(long long long long)
211 stdcall MAPISendMail(long long ptr long long)
212 stub MAPISaveMail
213 stub MAPIReadMail
214 stub MAPIFindNext
215 stub MAPIDeleteMail
217 stub MAPIAddress
218 stub MAPIDetails
219 stub MAPIResolveName
220 stub BMAPISendMail
221 stub BMAPISaveMail
222 stub BMAPIReadMail
223 stub BMAPIGetReadMail
224 stub BMAPIFindNext
225 stub BMAPIAddress
226 stub BMAPIGetAddress
227 stub BMAPIDetails
228 stub BMAPIResolveName
229 stub cmc_act_on
230 stub cmc_free
231 stub cmc_list
232 stub cmc_logoff
233 stub cmc_logon
234 stub cmc_look_up
235 stdcall cmc_query_configuration( long long ptr ptr )
236 stub cmc_read
237 stub cmc_send
238 stub cmc_send_documents
239 stub HrDispatchNotifications@4
241 stub HrValidateParameters@8
244 stub ScCreateConversationIndex@16
246 stub HrGetOmiProvidersFlags
247 stub HrGetOmiProvidersFlags@8
248 stub HrSetOmiProvidersFlagsInvalid
249 stub HrSetOmiProvidersFlagsInvalid@4
250 stub GetOutlookVersion
251 stub GetOutlookVersion@0
252 stub FixMAPI
253 stub FixMAPI@0
254 stdcall FGetComponentPath(str str ptr long long)
255 stdcall FGetComponentPath@20(str str ptr long long) FGetComponentPath
