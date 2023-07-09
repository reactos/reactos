VERSION 4.00
Begin VB.Form browsectl 
   Caption         =   "Explorer in a form"
   ClientHeight    =   5745
   ClientLeft      =   4005
   ClientTop       =   2580
   ClientWidth     =   7920
   Height          =   6150
   Left            =   3945
   LinkTopic       =   "Form2"
   ScaleHeight     =   5745
   ScaleWidth      =   7920
   Top             =   2235
   Width           =   8040
   Begin VB.CommandButton Refresh 
      Caption         =   "Refresh"
      Height          =   375
      Left            =   6720
      TabIndex        =   8
      Top             =   120
      Width           =   735
   End
   Begin VB.CommandButton Stop 
      Caption         =   "Stop"
      Height          =   375
      Left            =   6240
      TabIndex        =   7
      Top             =   120
      Width           =   495
   End
   Begin VB.CommandButton Go 
      Caption         =   "Go"
      Height          =   375
      Left            =   3600
      TabIndex        =   6
      Top             =   120
      Width           =   495
   End
   Begin VB.CommandButton Search 
      Caption         =   "Search"
      Height          =   375
      Left            =   5520
      TabIndex        =   4
      Top             =   120
      Width           =   735
   End
   Begin VB.TextBox Location 
      Height          =   375
      Left            =   120
      TabIndex        =   3
      Top             =   120
      Width           =   3375
   End
   Begin VB.CommandButton Home 
      Caption         =   "Home"
      Height          =   375
      Left            =   4920
      TabIndex        =   2
      Top             =   120
      Width           =   615
   End
   Begin VB.CommandButton Forward 
      Caption         =   ">"
      Enabled         =   0   'False
      Height          =   375
      Left            =   4560
      TabIndex        =   1
      Top             =   120
      Width           =   375
   End
   Begin VB.CommandButton Back 
      Caption         =   "<"
      Enabled         =   0   'False
      Height          =   375
      Left            =   4200
      TabIndex        =   0
      Top             =   120
      Width           =   375
   End
   Begin SHDocVwCtl.WebBrowser WebBrowser1 
      Height          =   4455
      Left            =   120
      TabIndex        =   9
      Top             =   600
      Width           =   7575
      Object.Height          =   297
      Object.Width           =   505
      AutoSize        =   0
      ViewMode        =   1
      AutoSizePercentage=   0
      AutoArrange     =   -1  'True
      NoClientEdge    =   -1  'True
      AlignLeft       =   0   'False
      Location        =   "file://C:\WINDOWS\SYSTEM\BLANK.HTM"
   End
   Begin ComctlLib.StatusBar StatusBar 
      Align           =   2  'Align Bottom
      Height          =   495
      Left            =   0
      TabIndex        =   5
      Top             =   5250
      Width           =   7920
      _Version        =   65536
      _ExtentX        =   13970
      _ExtentY        =   873
      _StockProps     =   68
      AlignSet        =   -1  'True
      SimpleText      =   ""
      i1              =   "browsctl.frx":0000
   End
End
Attribute VB_Name = "browsectl"
Attribute VB_Creatable = False
Attribute VB_Exposed = False


Private Sub ShellFolderOC1_Click()

End Sub


Private Sub Back_Click()
    On Error GoTo Boom
    
    WebBrowser1.GoBack
    GoTo endfunc
Boom:
    Beep
endfunc:
   
End Sub



Private Sub Form_Resize()
   WebBrowser1.Width = Width - (WebBrowser1.Left * 2)
   Rem I wish I new the internal height instead of whole height
   WebBrowser1.Height = Height - StatusBar.Height - WebBrowser1.Top - (WebBrowser1.Top - (Location.Top + Location.Height)) - Location.Top - 2 * (WebBrowser1.Top - (Location.Top + Location.Height))
End Sub


Private Sub Forward_Click()
    On Error GoTo Boom
    WebBrowser1.GoForward
    GoTo endfunc
Boom:
    Beep
endfunc:

End Sub

Private Sub Go_Click()
    On Error GoTo Error:
    If KeyAscii = 13 Then
        WebBrowser1.Navigate (Location)
    End If
    Exit Sub
    
Error:
    StatusBar.Panels(1).Text = Err.Description + "(" + Hex(Err) + ")"
    Beep
End Sub

Private Sub Home_Click()
    On Error GoTo Boom
    
    WebBrowser1.GoHome
    GoTo endfunc
Boom:
    Beep
endfunc:
End Sub


Private Sub Location_KeyPress(KeyAscii As Integer)
    On Error GoTo Error:
    If KeyAscii = 13 Then
        WebBrowser1.Navigate (Location)
    End If
    Exit Sub
    
Error:
    StatusBar.Panels(1).Text = Err.Description + "(" + Hex(Err) + ")"
    Beep
End Sub


Private Sub Refresh_Click()
    On Error GoTo Boom
    WebBrowser1.Refresh
    GoTo endfunc
Boom:
    Beep
endfunc:


End Sub

Private Sub Search_Click()
    On Error GoTo Boom
    WebBrowser1.GoSearch
    GoTo endfunc
Boom:
    Beep
endfunc:

End Sub


Private Sub StatusBar1_PanelClick(ByVal Panel As Panel)

End Sub


Private Sub ShellExplorer1_OnBeginNavigate(ByVal HLink As Object, Cancel As Boolean)

End Sub

Private Sub Stop_Click()
    On Error GoTo Boom
    WebBrowser1.Stop
    GoTo endfunc
Boom:
    Beep
endfunc:

End Sub


Private Sub WebBrowser1_OnBeginNavigate(ByVal URL As String, ByVal Flags As Long, ByVal TargetFrameName As String, PostData As Variant, ByVal Headers As String, ByVal Referrer As String, Cancel As Boolean)
    On Error GoTo Boom:
    StatusBar.Panels(1).Text = HLink.Location
    GoTo Done
Boom:
    Beep
Done:
End Sub


Private Sub WebBrowser1_OnCommandStateChange(ByVal Command As Long, ByVal Enable As Boolean)
    Rem we need to define the commands some place
    If Command = 2 Then
        Back.Enabled = Enable
    ElseIf Command = 1 Then
        Forward.Enabled = Enable
    End If
End Sub


Private Sub WebBrowser1_OnDownloadComplete()
StatusBar.Panels(1).Text = "Page Complete"
End Sub


Private Sub WebBrowser1_OnNavigate(ByVal URL As String, ByVal Flags As Long, ByVal TargetFrameName As String, PostData As Variant, ByVal Headers As String, ByVal Referrer As String)
    On Error GoTo Boom
    Location = HLink.Location
    GoTo endfunc
Boom:
    Beep
endfunc:

End Sub


Private Sub WebBrowser1_OnStatusTextChange(ByVal bstrText As String)
    StatusBar.Panels(1).Text = bstrText
End Sub


Private Sub WebBrowser1_BeforeNavigate(ByVal URL As String, ByVal Flags As Long, ByVal TargetFrameName As String, PostData As Variant, ByVal Headers As String, Cancel As Boolean)
    StatusBar.Panels(1).Text = "Before Nav: " + URL
End Sub


Private Sub WebBrowser1_NavigateComplete(ByVal URL As String)
    StatusBar.Panels(1).Text = "End Nav: " + URL
End Sub


Private Sub WebBrowser1_StatusTextChange(ByVal Text As String)
    StatusBar.Panels(1).Text = Text
End Sub


