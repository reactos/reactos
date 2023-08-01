100 stub -noname EnumPrinterPropertySheets
101 stub -noname ClusterSplOpen
102 stub -noname ClusterSplClose
103 stub -noname ClusterSplIsAlive
104 stub PerfClose
105 stub PerfCollect
106 stub PerfOpen
107 stdcall ADVANCEDSETUPDIALOG(ptr long ptr ptr) AdvancedSetupDialog
108 stdcall AbortPrinter(ptr)
109 stdcall AddFormA(ptr long ptr)
110 stdcall AddFormW(ptr long ptr)
111 stdcall AddJobA(ptr long ptr long ptr)
112 stdcall AddJobW(ptr long ptr long ptr)
113 stdcall AddMonitorA(str long ptr)
114 stdcall AddMonitorW(wstr long ptr)
115 stdcall AddPortA(str ptr str)
116 stdcall AddPortExA(str long ptr str)
117 stdcall AddPortExW(wstr long ptr wstr)
118 stdcall AddPortW(wstr ptr wstr)
119 stdcall AddPrintProcessorA(str str str str)
120 stdcall AddPrintProcessorW(wstr wstr wstr wstr)
121 stdcall AddPrintProvidorA(str long ptr)
122 stdcall AddPrintProvidorW(wstr long ptr)
123 stdcall AddPrinterA(str long ptr)
124 stub AddPrinterConnectionA
125 stub AddPrinterConnectionW
126 stdcall AddPrinterDriverA(str long ptr)
127 stdcall AddPrinterDriverExA(str long ptr long)
128 stdcall AddPrinterDriverExW(wstr long ptr long)
129 stdcall AddPrinterDriverW(wstr long ptr)
130 stdcall AddPrinterW(wstr long ptr)
131 stdcall AdvancedDocumentPropertiesA(ptr ptr str ptr ptr)
132 stdcall AdvancedDocumentPropertiesW(ptr ptr wstr ptr ptr)
133 stdcall AdvancedSetupDialog(ptr long ptr ptr)
134 stdcall ClosePrinter(ptr)
135 stdcall CloseSpoolFileHandle(ptr ptr)
136 stdcall CommitSpoolData(ptr ptr long)
137 stdcall ConfigurePortA(str ptr str)
138 stdcall ConfigurePortW(wstr ptr wstr)
139 stub ConnectToPrinterDlg
140 stub ConvertAnsiDevModeToUnicodeDevmode
141 stub ConvertUnicodeDevModeToAnsiDevmode
142 stdcall CreatePrinterIC(ptr ptr)
143 stdcall DEVICECAPABILITIES(str str long ptr ptr) DeviceCapabilitiesA
144 stdcall DEVICEMODE(ptr ptr str ptr) DeviceMode
145 stdcall DeleteFormA(ptr str)
146 stdcall DeleteFormW(ptr wstr)
147 stdcall DeleteMonitorA(str str str)
148 stdcall DeleteMonitorW(wstr wstr wstr)
149 stdcall DeletePortA(str ptr str)
150 stdcall DeletePortW(wstr ptr wstr)
151 stdcall DeletePrintProcessorA(str str str)
152 stdcall DeletePrintProcessorW(wstr wstr wstr)
153 stdcall DeletePrintProvidorA(str str str)
154 stdcall DeletePrintProvidorW(wstr wstr wstr)
155 stdcall DeletePrinter(ptr)
156 stub DeletePrinterConnectionA
157 stub DeletePrinterConnectionW
158 stdcall DeletePrinterDataA(ptr str)
159 stdcall DeletePrinterDataExA(ptr str str)
160 stdcall DeletePrinterDataExW(ptr wstr wstr)
161 stdcall DeletePrinterDataW(ptr wstr)
162 stdcall DeletePrinterDriverA(str str str)
163 stdcall DeletePrinterDriverExA(str str str long long)
164 stdcall DeletePrinterDriverExW(wstr wstr wstr long long)
165 stdcall DeletePrinterDriverW(wstr wstr wstr)
166 stdcall DeletePrinterIC(ptr)
167 stdcall DeletePrinterKeyA(ptr str)
168 stdcall DeletePrinterKeyW(ptr wstr)
169 stdcall DevQueryPrint(ptr ptr ptr)
170 stdcall DevQueryPrintEx(ptr)
171 stdcall DeviceCapabilities(str str long ptr ptr) DeviceCapabilitiesA
172 stdcall DeviceCapabilitiesA(str str long ptr ptr)
173 stdcall DeviceCapabilitiesW(wstr wstr long ptr ptr)
174 stdcall DeviceMode(ptr ptr str ptr)
175 stdcall DevicePropertySheets(ptr long)
176 stdcall DocumentEvent(ptr ptr long long ptr long ptr)
177 stdcall DocumentPropertiesA(ptr ptr str ptr ptr long)
178 stdcall DocumentPropertiesW(ptr ptr wstr ptr ptr long)
179 stdcall DocumentPropertySheets(ptr long)
180 stdcall EXTDEVICEMODE(ptr ptr ptr str str ptr str long) ExtDeviceMode
181 stdcall EndDocPrinter(ptr)
182 stdcall EndPagePrinter(ptr)
183 stdcall EnumFormsA(ptr long ptr long ptr ptr)
184 stdcall EnumFormsW(ptr long ptr long ptr ptr)
185 stdcall EnumJobsA(ptr long long long ptr long ptr ptr)
186 stdcall EnumJobsW(ptr long long long ptr long ptr ptr)
187 stdcall EnumMonitorsA(str long ptr long ptr ptr)
188 stdcall EnumMonitorsW(wstr long ptr long ptr ptr)
189 stdcall EnumPortsA(str long ptr long ptr ptr)
190 stdcall EnumPortsW(wstr long ptr long ptr ptr)
191 stdcall EnumPrintProcessorDatatypesA(ptr ptr long ptr long ptr ptr)
192 stdcall EnumPrintProcessorDatatypesW(ptr ptr long ptr long ptr ptr)
193 stdcall EnumPrintProcessorsA(str str long ptr long ptr ptr)
194 stdcall EnumPrintProcessorsW(wstr wstr long ptr long ptr ptr)
195 stdcall EnumPrinterDataA(ptr long str long ptr ptr ptr long ptr)
196 stdcall EnumPrinterDataExA(ptr str ptr long ptr ptr)
197 stdcall EnumPrinterDataExW(ptr wstr ptr long ptr ptr)
198 stdcall EnumPrinterDataW(ptr long wstr long ptr ptr ptr long ptr)
199 stdcall EnumPrinterDriversA(str str long ptr long ptr ptr)
200 stdcall EnumPrinterDriversW(wstr wstr long ptr long ptr ptr)
201 stdcall GetDefaultPrinterA(ptr ptr)
202 stdcall SetDefaultPrinterA(str)
203 stdcall GetDefaultPrinterW(ptr ptr)
204 stdcall SetDefaultPrinterW(wstr)
205 stub -noname SplReadPrinter
206 stub -noname AddPerMachineConnectionA
207 stub -noname AddPerMachineConnectionW
208 stub -noname DeletePerMachineConnectionA
209 stub -noname DeletePerMachineConnectionW
210 stub -noname EnumPerMachineConnectionsA
211 stub -noname EnumPerMachineConnectionsW
212 stdcall -noname LoadPrinterDriver(ptr)
213 stub -noname RefCntLoadDriver
214 stub -noname RefCntUnloadDriver
215 stub -noname ForceUnloadDriver
216 stub -noname PublishPrinterA
217 stub -noname PublishPrinterW
218 stdcall -noname CallCommonPropertySheetUI(ptr ptr long ptr)
219 stub -noname PrintUIQueueCreate
220 stub -noname PrintUIPrinterPropPages
221 stub -noname PrintUIDocumentDefaults
222 stub -noname SendRecvBidiData
223 stub -noname RouterFreeBidiResponseContainer
224 stub -noname ExternalConnectToLd64In32Server
225 stdcall EnumPrinterKeyA(ptr str str long ptr)
226 stub -noname PrintUIWebPnpEntry
227 stub -noname PrintUIWebPnpPostEntry
228 stub -noname PrintUICreateInstance
229 stub -noname PrintUIDocumentPropertiesWrap
230 stub -noname PrintUIPrinterSetup
231 stub -noname PrintUIServerPropPages
232 stub -noname AddDriverCatalog
233 stdcall EnumPrinterKeyW(ptr wstr wstr long ptr)
234 stdcall EnumPrintersA(long ptr long ptr long ptr ptr)
235 stdcall EnumPrintersW(long ptr long ptr long ptr ptr)
236 stdcall ExtDeviceMode(ptr ptr ptr str str ptr str long)
237 stub FindClosePrinterChangeNotification
238 stub FindFirstPrinterChangeNotification
239 stub FindNextPrinterChangeNotification
240 stdcall FlushPrinter(ptr ptr long ptr long)
241 stub FreePrinterNotifyInfo
242 stdcall GetFormA(ptr str long ptr long ptr)
243 stdcall GetFormW(ptr str long ptr long ptr)
244 stdcall GetJobA(ptr long long ptr long ptr)
245 stdcall GetJobW(ptr long long ptr long ptr)
246 stdcall GetPrintProcessorDirectoryA(str str long ptr long ptr)
247 stdcall GetPrintProcessorDirectoryW(wstr wstr long ptr long ptr)
248 stdcall GetPrinterA(ptr long ptr long ptr)
249 stdcall GetPrinterDataA(ptr str ptr ptr long ptr)
250 stdcall GetPrinterDataExA(ptr str str ptr ptr long ptr)
251 stdcall GetPrinterDataExW(ptr wstr wstr ptr ptr long ptr)
252 stdcall GetPrinterDataW(ptr wstr ptr ptr long ptr)
253 stdcall GetPrinterDriverA(ptr str long ptr long ptr)
254 stdcall GetPrinterDriverDirectoryA(str str long ptr long ptr)
255 stdcall GetPrinterDriverDirectoryW(wstr wstr long ptr long ptr)
256 stdcall GetPrinterDriverW(ptr wstr long ptr long ptr)
257 stdcall GetPrinterW(ptr long ptr long ptr)
258 stdcall GetSpoolFileHandle(ptr)
259 stdcall IsValidDevmodeA(ptr long)
260 stdcall IsValidDevmodeW(ptr long)
261 stdcall OpenPrinterA(str ptr ptr)
262 stdcall OpenPrinterW(wstr ptr ptr)
263 stdcall PlayGdiScriptOnPrinterIC(ptr ptr long ptr long long)
264 stdcall PrinterMessageBoxA(ptr long ptr str str long)
265 stdcall PrinterMessageBoxW(ptr long ptr wstr wstr long)
266 stdcall PrinterProperties(ptr ptr)
267 stdcall QueryColorProfile(ptr ptr long ptr ptr ptr)
268 stdcall QueryRemoteFonts(ptr ptr long)
269 stdcall QuerySpoolMode(ptr ptr ptr)
270 stdcall ReadPrinter(ptr ptr long ptr)
271 stdcall ResetPrinterA(ptr ptr)
272 stdcall ResetPrinterW(ptr ptr)
273 stdcall ScheduleJob(ptr long)
274 stdcall SeekPrinter(ptr int64 ptr long long)
275 stub SetAllocFailCount
276 stdcall SetFormA(ptr str long str)
277 stdcall SetFormW(ptr str long str)
278 stdcall SetJobA(ptr long long ptr long)
279 stdcall SetJobW(ptr long long ptr long)
280 stdcall SetPortA(str str long ptr)
281 stdcall SetPortW(wstr wstr long ptr)
282 stdcall SetPrinterA(ptr long ptr long)
283 stdcall SetPrinterDataA(ptr str long ptr long)
284 stdcall SetPrinterDataExA(ptr str str long ptr long)
285 stdcall SetPrinterDataExW(ptr wstr wstr long ptr long)
286 stdcall SetPrinterDataW(ptr wstr long ptr long)
287 stdcall SetPrinterW(ptr long ptr long)
288 stdcall SplDriverUnloadComplete(ptr)
289 stub SpoolerDevQueryPrintW
290 stdcall SpoolerInit()
291 stdcall SpoolerPrinterEvent(wstr long long long)
292 stdcall StartDocDlgA(ptr ptr)
293 stdcall StartDocDlgW(ptr ptr)
294 stdcall StartDocPrinterA(ptr long ptr)
295 stdcall StartDocPrinterW(ptr long ptr)
296 stdcall StartPagePrinter(ptr)
297 stub WaitForPrinterChange
298 stdcall WritePrinter(ptr ptr long ptr)
299 stdcall XcvDataW(ptr wstr ptr long ptr long ptr ptr)
