VERSION 4.00
Begin VB.Form browsectl 
   Caption         =   "Explorer in a form"
   ClientHeight    =   5745
   ClientLeft      =   2745
   ClientTop       =   1455
   ClientWidth     =   7920
   Height          =   6150
   Left            =   2685
   LinkTopic       =   "Form2"
   ScaleHeight     =   5745
   ScaleWidth      =   7920
   Top             =   1110
   Width           =   8040
   Begin VB.CommandButton Search 
      Caption         =   "Search"
      Height          =   375
      Left            =   6240
      TabIndex        =   5
      Top             =   120
      Width           =   735
   End
   Begin VB.TextBox Location 
      Height          =   375
      Left            =   120
      TabIndex        =   4
      Top             =   120
      Width           =   3855
   End
   Begin VB.CommandButton Home 
      Caption         =   "Home"
      Height          =   375
      Left            =   5520
      TabIndex        =   3
      Top             =   120
      Width           =   735
   End
   Begin VB.CommandButton Forward 
      Caption         =   ">"
      Height          =   375
      Left            =   4920
      TabIndex        =   2
      Top             =   120
      Width           =   615
   End
   Begin VB.CommandButton Back 
      Caption         =   "<"
      Height          =   375
      Left            =   4200
      TabIndex        =   1
      Top             =   120
      Width           =   735
   End
   Begin ComctlLib.StatusBar StatusBar 
      Align           =   2  'Align Bottom
      Height          =   495
      Left            =   0
      TabIndex        =   6
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
   Begin SHDocVwCtl.ShellExplorer Explorer 
      Height          =   4455
      Left            =   120
      OleObjectBlob   =   "browsctl.frx":010C
      TabIndex        =   0
      Top             =   720
      Width           =   7575
   End
End
Attribute VB_Name = "browsectl"
Attribute VB_Creatable = False
Attribute VB_Exposed = False


Private Sub ShellFolderOC1_Click()

End Sub


Private Sub Back_Click()
    On Error GoTo Boom
    
    Explorer.GoBack
    GoTo endfunc
Boom:
    Beep
endfunc:
   
End Sub

Private Sub Explorer_OnBeginNavigate(ByVal HLink As Object, Cancel As Boolean)
    On Error GoTo Boom:
    StatusBar.Panels(1).Text = HLink.Location
    GoTo Done
Boom:
    Beep
Done:
    
End Sub

Private Sub Explorer_OnNavigate(ByVal HLink As Object)
    On Error GoTo Boom
    Location = HLink.Location
    GoTo endfunc
Boom:
    Beep
endfunc:
    
End Sub


Private Sub Explorer_OnStatusTextChange(ByVal bstrText As String)
    StatusBar.Panels(1).Text = bstrText
End Sub


Private Sub Form_Resize()
   Explorer.Width = Width - (Explorer.Left * 2)
   Explorer.Height = Height - (StatusBar.Height + Explorer.Top + (2 * (Explorer.Top - (Location.Top + Location.Height))))
End Sub


Private Sub Forward_Click()
    On Error GoTo Boom
    Explorer.GoForward
    GoTo endfunc
Boom:
    Beep
endfunc:

End Sub

Private Sub Home_Click()
    On Error GoTo Boom
    
    Explorer.GoHome
    GoTo endfunc
Boom:
    Beep
endfunc:
End Sub


Private Sub Location_KeyPress(KeyAscii As Integer)
    If KeyAscii = 13 Then
        Explorer.Location = Location
    End If

End Sub


Private Sub Search_Click()
    On Error GoTo Boom
    Explorer.GoSearch
    GoTo endfunc
Boom:
    Beep
endfunc:

End Sub


Private Sub StatusBar1_PanelClick(ByVal Panel As Panel)

End Sub


