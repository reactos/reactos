100 stub -noname EnumPrinterPropertySheets
101 stub -noname ClusterSplOpen
102 stub -noname ClusterSplClose
103 stub -noname ClusterSplIsAlive
104 stub PerfClose
105 stub PerfCollect
106 stub PerfOpen
107 stub ADVANCEDSETUPDIALOG
108 stub AbortPrinter
109 stub AddFormA
110 stub AddFormW
111 stub AddJobA
112 stdcall AddJobW(long long ptr long ptr)
113 stub AddMonitorA
114 stub AddMonitorW
115 stub AddPortA
116 stub AddPortExA
117 stub AddPortExW
118 stub AddPortW
119 stub AddPrintProcessorA
120 stub AddPrintProcessorW
121 stub AddPrintProvidorA
122 stub AddPrintProvidorW
123 stub AddPrinterA
124 stub AddPrinterConnectionA
125 stub AddPrinterConnectionW
126 stub AddPrinterDriverA
127 stub AddPrinterDriverExA
128 stub AddPrinterDriverExW
129 stub AddPrinterDriverW
130 stub AddPrinterW
131 stub AdvancedDocumentPropertiesA
132 stub AdvancedDocumentPropertiesW
133 stub AdvancedSetupDialog
134 stdcall ClosePrinter(long)
135 stub CloseSpoolFileHandle
136 stub CommitSpoolData
137 stub ConfigurePortA
138 stub ConfigurePortW
139 stub ConnectToPrinterDlg
140 stub ConvertAnsiDevModeToUnicodeDevmode
141 stub ConvertUnicodeDevModeToAnsiDevmode
142 stub CreatePrinterIC
143 stub DEVICECAPABILITIES
144 stub DEVICEMODE
145 stub DeleteFormA
146 stub DeleteFormW
147 stub DeleteMonitorA
148 stub DeleteMonitorW
149 stub DeletePortA
150 stub DeletePortW
151 stub DeletePrintProcessorA
152 stub DeletePrintProcessorW
153 stub DeletePrintProvidorA
154 stub DeletePrintProvidorW
155 stub DeletePrinter
156 stub DeletePrinterConnectionA
157 stub DeletePrinterConnectionW
158 stub DeletePrinterDataA
159 stub DeletePrinterDataExA
160 stub DeletePrinterDataExW
161 stub DeletePrinterDataW
162 stub DeletePrinterDriverA
163 stub DeletePrinterDriverExA
164 stub DeletePrinterDriverExW
165 stub DeletePrinterDriverW
166 stub DeletePrinterIC
167 stub DeletePrinterKeyA
168 stub DeletePrinterKeyW
169 stub DevQueryPrint
170 stub DevQueryPrintEx
171 stub DeviceCapabilities
172 stdcall DeviceCapabilitiesA(str str long ptr ptr)
173 stdcall DeviceCapabilitiesW(wstr wstr long ptr ptr)
174 stub DeviceMode
175 stub DevicePropertySheets
176 stub DocumentEvent
177 stdcall DocumentPropertiesA(long long ptr ptr ptr long)
178 stdcall DocumentPropertiesW(long long ptr ptr ptr long)
179 stub DocumentPropertySheets
180 stub EXTDEVICEMODE
181 stdcall EndDocPrinter(long)
182 stdcall EndPagePrinter(long)
183 stub EnumFormsA
184 stub EnumFormsW
185 stub EnumJobsA
186 stub EnumJobsW
187 stub EnumMonitorsA
188 stub EnumMonitorsW
189 stub EnumPortsA
190 stub EnumPortsW
191 stdcall EnumPrintProcessorDatatypesA(ptr ptr long ptr long ptr ptr)
192 stdcall EnumPrintProcessorDatatypesW(ptr ptr long ptr long ptr ptr)
193 stub EnumPrintProcessorsA
194 stdcall EnumPrintProcessorsW(ptr ptr long ptr long ptr ptr)
195 stub EnumPrinterDataA
196 stub EnumPrinterDataExA
197 stub EnumPrinterDataExW
198 stub EnumPrinterDataW
199 stub EnumPrinterDriversA
200 stub EnumPrinterDriversW
201 stdcall GetDefaultPrinterA(ptr ptr)
202 stub SetDefaultPrinterA
203 stdcall GetDefaultPrinterW(ptr ptr)
204 stub SetDefaultPrinterW
205 stub -noname SplReadPrinter
206 stub -noname AddPerMachineConnectionA
207 stub -noname AddPerMachineConnectionW
208 stub -noname DeletePerMachineConnectionA
209 stub -noname DeletePerMachineConnectionW
210 stub -noname EnumPerMachineConnectionsA
211 stub -noname EnumPerMachineConnectionsW
212 stub -noname LoadPrinterDriver
213 stub -noname RefCntLoadDriver
214 stub -noname RefCntUnloadDriver
215 stub -noname ForceUnloadDriver
216 stub -noname PublishPrinterA
217 stub -noname PublishPrinterW
218 stub -noname CallCommonPropertySheetUI
219 stub -noname PrintUIQueueCreate
220 stub -noname PrintUIPrinterPropPages
221 stub -noname PrintUIDocumentDefaults
222 stub -noname SendRecvBidiData
223 stub -noname RouterFreeBidiResponseContainer
224 stub -noname ExternalConnectToLd64In32Server
225 stub EnumPrinterKeyA
226 stub -noname PrintUIWebPnpEntry
227 stub -noname PrintUIWebPnpPostEntry
228 stub -noname PrintUICreateInstance
229 stub -noname PrintUIDocumentPropertiesWrap
230 stub -noname PrintUIPrinterSetup
231 stub -noname PrintUIServerPropPages
232 stub -noname AddDriverCatalog
233 stub EnumPrinterKeyW
234 stdcall EnumPrintersA(long ptr long ptr long ptr ptr)
235 stdcall EnumPrintersW(long ptr long ptr long ptr ptr)
236 stub ExtDeviceMode
237 stub FindClosePrinterChangeNotification
238 stub FindFirstPrinterChangeNotification
239 stub FindNextPrinterChangeNotification
240 stub FlushPrinter
241 stub FreePrinterNotifyInfo
242 stub GetFormA
243 stub GetFormW
244 stub GetJobA
245 stdcall GetJobW(long long long ptr long ptr)
246 stub GetPrintProcessorDirectoryA
247 stdcall GetPrintProcessorDirectoryW(wstr wstr long ptr long ptr)
248 stdcall GetPrinterA(long long ptr long ptr)
249 stub GetPrinterDataA
250 stub GetPrinterDataExA
251 stub GetPrinterDataExW
252 stub GetPrinterDataW
253 stdcall GetPrinterDriverA(long str long ptr long ptr)
254 stub GetPrinterDriverDirectoryA
255 stub GetPrinterDriverDirectoryW
256 stdcall GetPrinterDriverW(long wstr long ptr long ptr)
257 stdcall GetPrinterW(long long ptr long ptr)
258 stub GetSpoolFileHandle
259 stub IsValidDevmodeA
260 stub IsValidDevmodeW
261 stdcall OpenPrinterA(str ptr ptr)
262 stdcall OpenPrinterW(wstr ptr ptr)
263 stub PlayGdiScriptOnPrinterIC
264 stub PrinterMessageBoxA
265 stub PrinterMessageBoxW
266 stub PrinterProperties
267 stub QueryColorProfile
268 stub QueryRemoteFonts
269 stub QuerySpoolMode
270 stub ReadPrinter
271 stub ResetPrinterA
272 stub ResetPrinterW
273 stub ScheduleJob
274 stub SeekPrinter
275 stub SetAllocFailCount
276 stub SetFormA
277 stub SetFormW
278 stub SetJobA
279 stub SetJobW
280 stub SetPortA
281 stub SetPortW
282 stub SetPrinterA
283 stub SetPrinterDataA
284 stub SetPrinterDataExA
285 stub SetPrinterDataExW
286 stub SetPrinterDataW
287 stub SetPrinterW
288 stub SplDriverUnloadComplete
289 stub SpoolerDevQueryPrintW
290 stdcall SpoolerInit()
291 stub SpoolerPrinterEvent
292 stub StartDocDlgA
293 stub StartDocDlgW
294 stub StartDocPrinterA
295 stdcall StartDocPrinterW(long long ptr)
296 stdcall StartPagePrinter(long)
297 stub WaitForPrinterChange
298 stdcall WritePrinter(long ptr long ptr)
299 stdcall XcvDataW(long wstr ptr long ptr long ptr ptr)
