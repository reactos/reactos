/*
 * FILE:            notificationtest.c
 * PURPOSE:         Files notifications testing
 * NOTES:           MSDN code from: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365261%28v=vs.85%29.aspx
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#if defined(_tsplitpath) && !defined(_tsplitpath_s)
#define _tsplitpath_s(f, d,dl, p,pl, n,nl, e,el) _tsplitpath(f, d, p, n, e)
#else
#define _wsplitpath_s(f, d,dl, p,pl, n,nl, e,el) _wsplitpath(f, d, p, n, e)
#define _splitpath_s(f, d,dl, p,pl, n,nl, e,el) _splitpath(f, d, p, n, e)
#endif

void RefreshDirectory(LPTSTR);
void RefreshTree(LPTSTR);
void WatchDirectory(LPTSTR);

int _tmain(int argc, TCHAR *argv[])
{
    if(argc != 2)
    {
        _tprintf(TEXT("Usage: %s <dir>\n"), argv[0]);
        return -1;
    }

    WatchDirectory(argv[1]);

    return 0;
}

void WatchDirectory(LPTSTR lpDir)
{
   DWORD dwWaitStatus;
   HANDLE dwChangeHandles[2];
   TCHAR lpDrive[4];
   TCHAR lpFile[_MAX_FNAME];
   TCHAR lpExt[_MAX_EXT];

   _tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

   lpDrive[2] = (TCHAR)'\\';
   lpDrive[3] = (TCHAR)'\0';

// Watch the directory for file creation and deletion.

   dwChangeHandles[0] = FindFirstChangeNotification(
      lpDir,                         // directory to watch
      FALSE,                         // do not watch subtree
      FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes

   if (dwChangeHandles[0] == INVALID_HANDLE_VALUE)
   {
     printf("\n ERROR: FindFirstChangeNotification function failed.\n");
     ExitProcess(GetLastError());
   }

// Watch the subtree for directory creation and deletion.

   dwChangeHandles[1] = FindFirstChangeNotification(
      lpDrive,                       // directory to watch
      TRUE,                          // watch the subtree
      FILE_NOTIFY_CHANGE_DIR_NAME);  // watch dir name changes

   if (dwChangeHandles[1] == INVALID_HANDLE_VALUE)
   {
     printf("\n ERROR: FindFirstChangeNotification function failed.\n");
     ExitProcess(GetLastError());
   }


// Make a final validation check on our handles.

   if ((dwChangeHandles[0] == NULL) || (dwChangeHandles[1] == NULL))
   {
     printf("\n ERROR: Unexpected NULL from FindFirstChangeNotification.\n");
     ExitProcess(GetLastError());
   }

// Change notification is set. Now wait on both notification
// handles and refresh accordingly.

   while (TRUE)
   {
   // Wait for notification.

      printf("\nWaiting for notification...\n");

      dwWaitStatus = WaitForMultipleObjects(2, dwChangeHandles,
         FALSE, INFINITE);

      switch (dwWaitStatus)
      {
         case WAIT_OBJECT_0:

         // A file was created, renamed, or deleted in the directory.
         // Refresh this directory and restart the notification.

             RefreshDirectory(lpDir);
             if ( FindNextChangeNotification(dwChangeHandles[0]) == FALSE )
             {
               printf("\n ERROR: FindNextChangeNotification function failed.\n");
               ExitProcess(GetLastError());
             }
             break;

         case WAIT_OBJECT_0 + 1:

         // A directory was created, renamed, or deleted.
         // Refresh the tree and restart the notification.

             RefreshTree(lpDrive);
             if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE )
             {
               printf("\n ERROR: FindNextChangeNotification function failed.\n");
               ExitProcess(GetLastError());
             }
             break;

         case WAIT_TIMEOUT:

         // A timeout occurred, this would happen if some value other
         // than INFINITE is used in the Wait call and no changes occur.
         // In a single-threaded environment you might not want an
         // INFINITE wait.

            printf("\nNo changes in the timeout period.\n");
            break;

         default:
            printf("\n ERROR: Unhandled dwWaitStatus.\n");
            ExitProcess(GetLastError());
            break;
      }
   }
}

void RefreshDirectory(LPTSTR lpDir)
{
   // This is where you might place code to refresh your
   // directory listing, but not the subtree because it
   // would not be necessary.

   _tprintf(TEXT("Directory (%s) changed.\n"), lpDir);
}

void RefreshTree(LPTSTR lpDrive)
{
   // This is where you might place code to refresh your
   // directory listing, including the subtree.

   _tprintf(TEXT("Directory tree (%s) changed.\n"), lpDrive);
}
