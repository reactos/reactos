VERSION 4.00
Begin VB.Form Form1 
   Caption         =   "Internet Explorer Test"
   ClientHeight    =   5325
   ClientLeft      =   2295
   ClientTop       =   2610
   ClientWidth     =   6600
   Height          =   5730
   Left            =   2235
   LinkTopic       =   "Form1"
   ScaleHeight     =   355
   ScaleMode       =   3  'Pixel
   ScaleWidth      =   440
   Top             =   2265
   Width           =   6720
   Begin VB.CommandButton HtmlShell 
      Caption         =   "html Shell?"
      Height          =   495
      Left            =   5040
      TabIndex        =   28
      Top             =   2400
      Width           =   975
   End
   Begin VB.CommandButton cliwin 
      Caption         =   "Client to Window"
      Height          =   495
      Left            =   4680
      TabIndex        =   27
      Top             =   960
      Width           =   1215
   End
   Begin VB.CommandButton PropGet 
      Caption         =   "Get"
      Height          =   375
      Left            =   5160
      TabIndex        =   26
      Top             =   3960
      Width           =   495
   End
   Begin VB.CommandButton PropSet 
      Caption         =   "set"
      Height          =   375
      Left            =   4560
      TabIndex        =   25
      Top             =   3960
      Width           =   495
   End
   Begin VB.TextBox PropValue 
      Height          =   495
      Left            =   2760
      TabIndex        =   24
      Top             =   3840
      Width           =   1695
   End
   Begin VB.TextBox PropName 
      Height          =   495
      Left            =   1200
      TabIndex        =   22
      Top             =   3840
      Width           =   1215
   End
   Begin VB.Timer Timer1 
      Interval        =   500
      Left            =   5880
      Top             =   3960
   End
   Begin VB.CommandButton TalkToHTML 
      Caption         =   "Talk to html"
      Height          =   495
      Left            =   3960
      TabIndex        =   19
      Top             =   2400
      Width           =   975
   End
   Begin VB.CommandButton DocType 
      Caption         =   "Doc Type"
      Height          =   495
      Left            =   2880
      TabIndex        =   18
      Top             =   2400
      Width           =   975
   End
   Begin VB.CommandButton FileName 
      Caption         =   "File Name"
      Height          =   495
      Left            =   1560
      TabIndex        =   17
      Top             =   2400
      Width           =   1215
   End
   Begin VB.CommandButton Path 
      Caption         =   "Path"
      Height          =   495
      Left            =   120
      TabIndex        =   16
      Top             =   2400
      Width           =   1215
   End
   Begin VB.ComboBox CoordList 
      Height          =   315
      ItemData        =   "form1.frx":0000
      Left            =   3360
      List            =   "form1.frx":001F
      TabIndex        =   15
      Top             =   3120
      Width           =   1215
   End
   Begin VB.CommandButton SetCoord 
      Caption         =   "Set"
      Height          =   255
      Left            =   4680
      TabIndex        =   14
      Top             =   3120
      Width           =   495
   End
   Begin VB.CommandButton GetCoord 
      Caption         =   "Get"
      Height          =   255
      Left            =   5160
      TabIndex        =   13
      Top             =   3120
      Width           =   495
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Test Excel Macro"
      Height          =   495
      Left            =   1560
      TabIndex        =   12
      Top             =   3120
      Width           =   1695
   End
   Begin VB.CommandButton IsShown 
      Caption         =   "Is Shown?"
      Height          =   495
      Left            =   4440
      TabIndex        =   11
      Top             =   1680
      Width           =   1215
   End
   Begin VB.CommandButton Hide 
      Caption         =   "Hide"
      Height          =   495
      Left            =   3720
      TabIndex        =   10
      Top             =   1680
      Width           =   615
   End
   Begin VB.CommandButton Show 
      Caption         =   "Show"
      Height          =   495
      Left            =   3000
      TabIndex        =   9
      Top             =   1680
      Width           =   615
   End
   Begin VB.CommandButton FullName 
      Caption         =   "Full Name"
      Height          =   495
      Left            =   1560
      TabIndex        =   8
      Top             =   1680
      Width           =   1215
   End
   Begin VB.CommandButton Name 
      Caption         =   "Name"
      Height          =   495
      Left            =   120
      TabIndex        =   6
      Top             =   1680
      Width           =   1215
   End
   Begin VB.CommandButton Quit 
      Caption         =   "&Quit"
      Height          =   495
      Left            =   120
      TabIndex        =   5
      Top             =   3120
      Width           =   1215
   End
   Begin VB.CommandButton Open 
      Caption         =   "&Open"
      Height          =   495
      Left            =   3120
      TabIndex        =   4
      Top             =   960
      Width           =   1215
   End
   Begin VB.TextBox OpenPath 
      Height          =   495
      Left            =   120
      TabIndex        =   3
      Top             =   960
      Width           =   2895
   End
   Begin VB.CommandButton GoHome 
      Caption         =   "Go Home"
      Height          =   495
      Left            =   3000
      TabIndex        =   2
      Top             =   240
      Width           =   1215
   End
   Begin VB.CommandButton GoForward 
      Caption         =   "Go Forward"
      Height          =   495
      Left            =   1560
      TabIndex        =   1
      Top             =   240
      Width           =   1215
   End
   Begin VB.CommandButton GoBack 
      Caption         =   "Go Back"
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   240
      Width           =   1215
   End
   Begin VB.Label Label2 
      Caption         =   "="
      Height          =   495
      Left            =   2520
      TabIndex        =   23
      Top             =   3840
      Width           =   135
   End
   Begin VB.Label Label1 
      Caption         =   "Properties"
      Height          =   495
      Left            =   120
      TabIndex        =   21
      Top             =   3840
      Width           =   855
   End
   Begin VB.Label busy 
      Height          =   495
      Left            =   5640
      TabIndex        =   20
      Top             =   4680
      Width           =   1095
   End
   Begin VB.Label Status 
      Height          =   495
      Left            =   120
      TabIndex        =   7
      Top             =   4680
      Width           =   5175
   End
End
Attribute VB_Name = "Form1"
Attribute VB_Creatable = False
Attribute VB_Exposed = False





Private Sub Command1_Click()

End Sub


Private Sub Combo1_Change()

End Sub

Private Sub cliwin_Click()
    On Error GoTo Boom
    x = 100
    y = 100
    Call iexplore.ClientToWindow(x, y)
    Status = "X=" + x + " y=" + y
    GoTo TheEnd
Boom:
    Beep
TheEnd:

End Sub

Private Sub Command2_Click()
Dim objXL As Excel.Application
Dim MyArray As Variant
On Error GoTo ErrorHandler
MyArray = Array(1, 2, 3)
Set objXL = IExplorer.Document
With objXL
    .Range("A1:c1").Value = MyArray
End With
GoTo endfunc
ErrorHandler:
    Beep
endfunc:
End Sub


Private Sub DocType_Click()
    On Error GoTo Boom
    Status = IExplorer.Type
    GoTo TheEnd
Boom:
    Beep
TheEnd:

End Sub

Private Sub filename_Click()
    On Error GoTo Boom
    Status = IExplorer.filename
    GoTo TheEnd
Boom:
    Beep
TheEnd:

End Sub

Private Sub Form_Load()
    Set IExplorer = CreateObject("InternetExplorer.Application")
    CoordList.ListIndex = 0
End Sub


Private Sub FullName_Click()
    Status = IExplorer.FullName
End Sub

Private Sub GetCoord_Click()
    Select Case CoordList.ListIndex
    Case 0
        Status = IExplorer.Left
    Case 1
        Status = IExplorer.Top
    Case 2
        Status = IExplorer.Width
    Case 3
        Status = IExplorer.Height
    Case 4
        Status = IExplorer.FullScreen
    Case 5
        Status = IExplorer.ToolBar
    Case 6
        Status = IExplorer.StatusBar
    Case 7
        Status = IExplorer.StatusText
    Case 8
        Status = IExplorer.MenuBar
        
    End Select
    
End Sub

Private Sub GoBack_Click()
    IExplorer.GoBack
End Sub


Private Sub GoForward_Click()
    IExplorer.GoForward
End Sub


Private Sub GoHome_Click()
    IExplorer.GoHome
End Sub


Private Sub Hide_Click()
    IExplorer.Visible = False
End Sub

Private Sub HtmlShell_Click()
Dim doc As Object
On Error GoTo ErrorHandler
Set doc = IExplorer.Document
Rem Status = doc.Script.Document.Forms.Count
Status = doc.Script.frames.Item(0).Document.links.Item(0).target
GoTo endfunc
ErrorHandler:
    Beep
endfunc:
End Sub

Private Sub IsShown_Click()
    If IExplorer.Visible Then
        Status = "Visible"
    Else
        Status = "Not Visible"
    End If
End Sub


Private Sub Name_Click()
    Status = IExplorer.Name
End Sub

Private Sub Open_Click()
    IExplorer.Open (OpenPath.Text)
End Sub


Private Sub Positions_Change(Index As Integer)

End Sub

Private Sub OpenPath_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
        IExplorer.Navigate (OpenPath.Text)
    End If
    
End Sub


Private Sub Path_Click()
    On Error GoTo Boom
    Status = IExplorer.Path
    GoTo TheEnd
Boom:
    Beep
TheEnd:
End Sub

Private Sub PropGet_Click()
   PropValue = IExplorer.GetProperty(PropName)
End Sub

Private Sub PropSet_Click()
    Dim val
    val = PropValue
    Call IExplorer.PutProperty(PropName, val)
End Sub

Private Sub Quit_Click()
    IExplorer.Quit
End Sub


Private Sub SetCoord_Click()
    On Error GoTo HandleError
    Select Case CoordList.ListIndex
    Case 0
        IExplorer.Left = OpenPath
    Case 1
        IExplorer.Top = OpenPath
    Case 2
        IExplorer.Width = OpenPath
    Case 3
        IExplorer.Height = OpenPath
    Case 4
        IExplorer.FullScreen = OpenPath
    Case 5
        IExplorer.ToolBar = OpenPath
    Case 6
        IExplorer.StatusBar = OpenPath
    Case 7
        IExplorer.StatusText = OpenPath
    Case 8
        IExplorer.MenuBar = OpenPath
    End Select
    GoTo endfunc
HandleError:
        Beep
endfunc:
End Sub

Private Sub Show_Click()
    IExplorer.Visible = True
End Sub


Private Sub TalkToHTML_Click()
Dim doc As Object
On Error GoTo ErrorHandler
Set doc = IExplorer.Document
Status = doc.Script.Location.HRef
GoTo endfunc
ErrorHandler:
    Beep
endfunc:

End Sub

Private Sub TestCtl_Click()
    browsectl.Show 0
End Sub


Private Sub Timer1_Timer()
    Busy = IExplorer.Busy
End Sub



