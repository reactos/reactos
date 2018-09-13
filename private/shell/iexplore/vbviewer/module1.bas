Attribute VB_Name = "Module1"
Public IExplorer As InternetExplorer

Public Sub EmptyFolderList()
    For i = 1 To FolderList.Count
        FolderList.Remove 1
    Next i
    
End Sub

Public Sub EmptyFolderItemList()
    For i = 1 To FolderItemList.Count
        FolderItemList.Remove 1
    Next i

End Sub
