Dim bDone : bDone = False
Dim Start : Start = 0
Dim Finish : Finish = 0
Dim ProgID : ProgID = "Forms.HTMLDoc.1"
Dim bPgDnTwice : bPgDnTwice = false

Sub Pad_DocLoaded(bLoaded)

	Finish = CurrentTime
	bDone = bLoaded
	
End Sub

Function ParseScriptParams

	Dim Pos : Pos = 0
	DIm PosEnd : posEnd = 0

	ParseScriptParams = ""
	ProgID = "Forms.HTMLDoc.1"
	bPgDnTwice = false

	pos = Instr (1, ScriptParam, ",")

	if pos > 0 then
		ParseScriptParams = Trim(Left(ScriptParam, pos-1))
		posEnd = Instr (pos+1, ScriptParam, ",")

		if posEnd > 0 then
			ProgID = Trim(Mid(ScriptParam, pos+1, posEnd-Pos-1))
			if Ucase(Trim(Right(ScriptParam, Len(ScriptParam)-posEnd))) = "TRUE" then
				bPgDnTwice = True
			end if
		else
			ProgID = Trim(Right(ScriptParam, Len(ScriptParam)-pos))
		end if
	end if

End Function

Sub Pad_Load()

	Dim File
	File = ParseScriptParams()

	Start = CurrentTime

	OpenFile File,  ProgID

	While Not bDone
		DoEvents
	Wend

	PrintLog Finish - Start
	
	Start = CurrentTime
	Sendkeys "{PGDN}", true
	Finish = CurrentTime
	PrintLog Finish - Start

	if bPgDnTwice then
		SendKeys "{PGUP}", true

		Start = CurrentTime
		Sendkeys "{PGDN}", true
		Sendkeys "{PGDN}", true
		Finish = CurrentTime
		PrintLog Finish - Start
	End if

End Sub