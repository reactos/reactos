/* Various utilities - NT versions
   Copyright (C) 1994, 1995, 1996 the Free Software Foundation.

   Written 1994, 1995, 1996 by:
   Juan Grigera, Miguel de Icaza, Janne Kukonlehto, Dugan Porter,
   Jakub Jelinek, Mauricio Plaza.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <config.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <signal.h>		/* my_system */
#include <limits.h>		/* INT_MAX */
#include <errno.h>
#if defined(_MSC_VER)
#include <sys/time.h___>		/* select: timeout */
#else
#include <sys/time.h>		/* select: timeout */
#endif
#include <sys/param.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <process.h>
#include "../src/fs.h"
#include "../src/util.h"
#include "util_win32.h"

#ifdef __BORLANDC__
#define ENOTEMPTY ERROR_DIR_NOT_EMPTY
#endif

char *get_owner (int uid)
{
    return "none";
}

char *get_group (int gid)
{
    return "none";
}

/* Pipes are guaranteed to be able to hold at least 4096 bytes */
/* More than that would be unportable */
#define MAX_PIPE_SIZE 4096

static int error_pipe[2];	/* File descriptors of error pipe */
static int old_error;		/* File descriptor of old standard error */

/* Creates a pipe to hold standard error for a later analysis. */
/* The pipe can hold 4096 bytes. Make sure no more is written */
/* or a deadlock might occur. */
void open_error_pipe (void)
{
    if (pipe (error_pipe) < 0){
	message (0, _(" Warning "), _(" Pipe failed ") );
    }
    old_error = dup (2);
    if(old_error < 0 || close(2) || dup (error_pipe[1]) != 2){
	message (0, _(" Warning "), _(" Dup failed ") );
	close (error_pipe[0]);
	close (error_pipe[1]);
    }
    close (error_pipe[1]);
}

void close_error_pipe (int error, char *text)
{
    char *title;
    char msg[MAX_PIPE_SIZE];
    int len = 0;

    if (error)
	title = _(" Error ");
    else
	title = _(" Warning ");
    if (old_error >= 0){
	close (2);
	dup (old_error);
	close (old_error);
	len = read (error_pipe[0], msg, MAX_PIPE_SIZE);

	if (len >= 0)
	    msg[len] = 0;
	close (error_pipe[0]);
    }
    if (error < 0)
	return;		/* Just ignore error message */
    if (text == NULL){
	if (len == 0) return;	/* Nothing to show */

	/* Show message from pipe */
	message (error, title, msg);
    } else {
	/* Show given text and possible message from pipe */
	message (error, title, " %s \n %s ", text, msg);
    }
}

void check_error_pipe (void)
{
    char error[MAX_PIPE_SIZE];
    int len = 0;
    if (old_error >= 0){
	while (len < MAX_PIPE_SIZE)
	{
	    int rvalue;

	    rvalue = -1; // read (error_pipe[0], error + len, 1);
	    if (rvalue <= 0)
		break;
	    len ++;
	}
	error[len] = 0;
	close (error_pipe[0]);
    }
    if (len > 0)
        message (0, _(" Warning "), error);
}

int my_system (int as_shell_command, const char *shell, const char *command)
{
    int status = 0;

#if 0
/* .ado: temp. turn out */
    if (as_shell_command) {
		/* It is only the shell, /c will not work */
		if (command) 
			spawnlp (P_WAIT, shell, shell, "/c", command, (char *) 0);
		else
			spawnlp (P_WAIT, shell, (char *) 0);
	} else
       spawnl (P_WAIT, shell, shell, command, (char *) 0);

    if (win32_GetPlatform() == OS_Win95) {
	    SetConsoleTitle ("GNU Midnight Commander");		/* title is gone after spawn... */
    }
#endif
    if (as_shell_command) {
	if (!access(command, 0)) {
	    switch(win32_GetEXEType (shell)) {	
		case EXE_win16:			/* Windows 3.x archive or OS/2 */
		case EXE_win32GUI:		/* NT or Chicago GUI API */
		    spawnlp (P_NOWAIT, shell, shell, "/c", command, (char *) 0);   /* don't wait for GUI programs to end */
		    break;			
		case EXE_otherCUI:		/* DOS COM, MZ, ZM, Phar Lap */
		case EXE_win32CUI:		/* NT or Chicago Console API, also OS/2 */
		case EXE_Unknown:
		default:
		    spawnlp (P_WAIT, shell, shell, "/c", command, (char *) 0);
		    break;
	    }
	}
	else
	    spawnlp (P_WAIT, shell, shell, "/c", command, (char *) 0);
    }
    else
       spawnl (P_WAIT, shell, shell, command, (char *) 0);

    if (win32_GetPlatform() == OS_Win95) {
	    SetConsoleTitle ("GNU Midnight Commander");		/* title is gone after spawn... */
    }

    return status;
}

/* get_default_shell
   Get the default shell for the current hardware platform
*/
char* get_default_shell()
{
    if (win32_GetPlatform() == OS_WinNT) 
	return "cmd.exe";
    else
	return "command.com";
}

char *tilde_expand (char *directory)
{
    return strdup (directory);
}

/* sleep: Call Windows API.
	  Can't do simple define. That would need <windows.h> in every source
*/
#ifndef __EMX__
void sleep(unsigned long dwMiliSecs)
{
    Sleep(dwMiliSecs);
}
#endif

/* Canonicalize path, and return a new path. Do everything in situ.
   The new path differs from path in:
	Multiple `/'s are collapsed to a single `/'.
	Leading `./'s and trailing `/.'s are removed.
	Trailing `/'s are removed.
	Non-leading `../'s and trailing `..'s are handled by removing
	portions of the path. */
char *canonicalize_pathname (char *path)
{
    int i, start;
    char stub_char;

    stub_char = (*path == PATH_SEP) ? PATH_SEP : '.';

    /* Walk along path looking for things to compact. */
    i = 0;
    for (;;) {
        if (!path[i])
	    break;

      	while (path[i] && path[i] != PATH_SEP)
	    i++;

      	start = i++;

      	/* If we didn't find any slashes, then there is nothing left to do. */
      	if (!path[start])
	    break;

        /* Handle multiple `/'s in a row. */
        while (path[i] == PATH_SEP)
	    i++;

        if ((start + 1) != i) {
	    strcpy (path + start + 1, path + i);
	    i = start + 1;
	}

        /* Handle backquoted `/'. */
        if (start > 0 && path[start - 1] == '\\')
	    continue;

        /* Check for trailing `/'. */
        if (start && !path[i]) {
	zero_last:
	    path[--i] = '\0';
	    break;
	}

        /* Check for `../', `./' or trailing `.' by itself. */
        if (path[i] == '.') {
	    /* Handle trailing `.' by itself. */
	    if (!path[i + 1])
	        goto zero_last;

	    /* Handle `./'. */
	    if (path[i + 1] == PATH_SEP) {
	        strcpy (path + i, path + i + 1);
	        i = start;
	        continue;
	    }

	    /* Handle `../' or trailing `..' by itself. 
	       Remove the previous ?/ part with the exception of
	       ../, which we should leave intact. */
	    if (path[i + 1] == '.' && (path[i + 2] == PATH_SEP || !path[i + 2])) {
	        while (--start > -1 && path[start] != PATH_SEP);
	        if (!strncmp (path + start + 1, "../", 3))
	            continue;
	        strcpy (path + start + 1, path + i + 2);
	        i = start;
	        continue;
	    }
	}
    }

    if (!*path) {
        *path = stub_char;
        path[1] = '\0';
    }
    return path;
}

#ifndef USE_VFS
/* 
   int mc_rmdir (char *path);
   Fix for Win95 UGLY BUG in rmdir: it will return ENOACCESS instead
   of ENOTEMPTY.
 */
int mc_rmdir (char *path)
{
    if (win32_GetPlatform() == OS_Win95) {
		if (rmdir(path)) {
			SetLastError (ERROR_DIR_NOT_EMPTY);
#ifndef __EMX__
			/* FIXME: We are always saying the same thing! */
			_doserrno = ERROR_DIR_NOT_EMPTY;
#endif
			errno = ENOTEMPTY;
			return -1;
		} else
			return 0;
    }
    else
		return rmdir(path);		/* No trouble in Windows NT */
}

static int conv_nt_unx_rc(int rc)
{
   int errCode;
   switch (rc) {
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
      case ERROR_TOO_MANY_OPEN_FILES:
         errCode = ENOENT;
         break;
      case ERROR_INVALID_HANDLE:
      case ERROR_ARENA_TRASHED:
      case ERROR_ACCESS_DENIED:
  	  case ERROR_INVALID_ACCESS:
  	  case ERROR_WRITE_PROTECT:
  	  case ERROR_WRITE_FAULT:
      case ERROR_READ_FAULT:
  	  case ERROR_SHARING_VIOLATION:
	     errCode = EACCES;
         break;
      case ERROR_NOT_ENOUGH_MEMORY:
		  errCode = ENOMEM;
		  break;
      case ERROR_INVALID_BLOCK:
	  case ERROR_INVALID_FUNCTION:
	  case ERROR_INVALID_DRIVE:
		  errCode = ENODEV;
		  break;
	  case ERROR_CURRENT_DIRECTORY:
		  errCode = ENOTDIR;
		  break;
	  case ERROR_NOT_READY:
         errCode = EINVAL;
         break;
      default:
         errCode = EINVAL;
        break;
   } /* endswitch */
   return errCode;
}

/* 
   int mc_unlink (char *pathName)
   For Windows 95 and NT, files should be able to be deleted even
   if they don't have write-protection. We should build a question box
   like: Delete anyway? Yes <No> All
*/
int mc_unlink (char *pathName)
{
	char		*fileName;
	char		*trunced_name;
	static int  erase_all = 0;
	BOOL        rc;
	DWORD       returnError;

	rc = DeleteFile(pathName);
	returnError = GetLastError();
	if ((rc == FALSE) && (returnError == ERROR_ACCESS_DENIED)) {
		int result;
		if (!erase_all) {
			errno = conv_nt_unx_rc(returnError);
			trunced_name = name_trunc(pathName, 30);
			fileName = (char *) malloc(strlen(trunced_name) + 16);
			strcpy(fileName, _("File "));
			strcat(fileName, trunced_name);
			strcat(fileName, _(" protected"));
		    result = query_dialog(fileName, _("Delete anyway?"), 3, 3, _(" No "), _(" Yes "), _(" All in the future!"));
			free(fileName);

		    switch (result) {
		    case 0:
			   do_refresh ();
			   return -1;
		    case 1:
		       do_refresh ();
			   break;
		    case 2:
			   do_refresh ();
			   erase_all = 1;
			   break;
		    default:
			   do_refresh ();
			   return -1;
			   break;
		   }
		}

		chmod(pathName, S_IWRITE); /* make it writable */
		rc = DeleteFile(pathName);
		returnError = GetLastError();
		if (rc == FALSE) {
			errno = conv_nt_unx_rc(returnError);
			return -1;
		}
	}
	if (rc == TRUE) return 0;
	else 
		return -1;
}
#endif /*USE_VFS*/

void my_statfs (struct my_statfs *myfs_stats, char *path)
{
    int len = 0;
    DWORD lpSectorsPerCluster, lpBytesPerSector, lpFreeClusters, lpClusters;
       DWORD           lpMaximumComponentLength, lpFileSystemFlags;
       static char     lpVolumeNameBuffer[256], lpFileSystemNameBuffer[30];

       GetDiskFreeSpace(NULL, &lpSectorsPerCluster, &lpBytesPerSector,
			&lpFreeClusters, &lpClusters);

       /* KBytes available */
       myfs_stats->avail = (unsigned int)( ((double)lpSectorsPerCluster * lpBytesPerSector * lpFreeClusters) / 1024 );
       
       /* KBytes total */
       myfs_stats->total = (unsigned int)( ((double)lpSectorsPerCluster * lpBytesPerSector * lpClusters) / 1024 ); 
       myfs_stats->nfree = lpFreeClusters;
       myfs_stats->nodes = lpClusters;

       GetVolumeInformation(NULL, lpVolumeNameBuffer, 255, NULL,
			    &lpMaximumComponentLength, &lpFileSystemFlags,
			    lpFileSystemNameBuffer, 30);

       myfs_stats->mpoint = lpFileSystemNameBuffer;
       myfs_stats->device = lpVolumeNameBuffer;


       myfs_stats->type = GetDriveType(NULL);
       switch (myfs_stats->type) {
	   /*
	    * mmm. DeviceIoControl may fail if you are not root case
	    * F5_1Pt2_512,            5.25", 1.2MB,  512 bytes/sector
	    * myfs_stats->typename = "5.25\" 1.2MB"; break; case
	    * F3_1Pt44_512,           3.5",  1.44MB, 512 bytes/sector
	    * myfs_stats->typename = "3.5\" 1.44MB"; break; case
	    * F3_2Pt88_512,           3.5",  2.88MB, 512 bytes/sector
	    * myfs_stats->typename = "3.5\" 2.88MB"; break; case
	    * F3_20Pt8_512,           3.5",  20.8MB, 512 bytes/sector
	    * myfs_stats->typename = "3.5\" 20.8MB"; break; case
	    * F3_720_512,             3.5",  720KB,  512 bytes/sector
	    * myfs_stats->typename = "3.5\" 720MB"; break; case
	    * F5_360_512,             5.25", 360KB,  512 bytes/sector
	    * myfs_stats->typename = "5.25\" 360KB"; break; case
	    * F5_320_512,             5.25", 320KB,  512 bytes/sector
	    * case F5_320_1024,       5.25", 320KB,  1024
	    * bytes/sector myfs_stats->typename = "5.25\" 320KB"; break;
	    * case F5_180_512,        5.25", 180KB,  512
	    * bytes/sector myfs_stats->typename = "5.25\" 180KB"; break;
	    * case F5_160_512,        5.25", 160KB,  512
	    * bytes/sector myfs_stats->typename = "5.25\" 160KB"; break;
	    * case RemovableMedia,    Removable media other than
	    * floppy myfs_stats->typename = "Removable"; break; case
	    * FixedMedia              Fixed hard disk media
	    * myfs_stats->typename = "Hard Disk"; break; case Unknown:
	    * Format is unknown
	    */
       case DRIVE_REMOVABLE:
               myfs_stats->typename = _("Removable");
               break;
       case DRIVE_FIXED:
               myfs_stats->typename = _("Hard Disk");
               break;
       case DRIVE_REMOTE:
               myfs_stats->typename = _("Networked");
               break;
       case DRIVE_CDROM:
               myfs_stats->typename = _("CD-ROM");
               break;
       case DRIVE_RAMDISK:
               myfs_stats->typename = _("RAM disk");
               break;
       default:
               myfs_stats->typename = _("unknown");
               break;
       };
}

int gettimeofday (struct timeval* tvp, void *p)
{
    if (p != NULL)
	return 0;	
    
 /* Since MC only calls this func from get_random_hint we return 
    some value, not exactly the "correct" one */
    tvp->tv_sec = GetTickCount()/1000; 	/* Number of milliseconds since Windows //started*/
    tvp->tv_usec = GetTickCount();
}

/* FAKE functions */

int 
look_for_exe(const char* pathname)
{
   int j;
   char *p;
   int lgh = strlen(pathname);

   if (lgh < 4) {
      return 0;
   } else {
      p = (char *) pathname;
      for (j=0; j<lgh-4; j++) {
         p++;
      } /* endfor */
      if (!stricmp(p, ".exe") || 
          !stricmp(p, ".bat") || 
          !stricmp(p, ".com") || 
          !stricmp(p, ".cmd")) {
         return 1;
      }
   }
   return 0;
}

int 
lstat (const char* pathname, struct stat *buffer)
{
   int rc = stat (pathname, buffer);
#ifdef __BORLANDC__
   if (rc == 0) {
     if (!(buffer->st_mode & S_IFDIR)) {
        if (!look_for_exe(pathname)) {
           buffer->st_mode &= !S_IXUSR & !S_IXGRP & !S_IXOTH;
	}
     }
   }
#endif
   return rc;
}

int getuid ()	      
{
/*    SID sid;
    LookupAccountName (NULL, &sid...
    return 0;
*/
    return 0;
}

int getgid ()	      
{
    return 0;
}

int readlink (char* path, char* buf, int size)
{
    return -1;
}
int symlink (char *n1, char *n2)
{
    return -1;
}
int link (char *p1, char *p2)
{
    return -1;
}
int chown (char *path, int owner, int group)
{
    return -1;
}
int mknod (char *path, int mode, int dev)
{
    return -1;
}

void init_uid_gid_cache (void)
{
    return;
}

/* INHANDLE is a result of some mc_open call to any vfs, this function
   returns a normal handle (to be used with read) of a pipe for reading
   of the output of COMMAND with arguments ... (must include argv[0] as
   well) which gets as its input at most INLEN bytes from the INHANDLE
   using mc_read. You have to call mc_doublepclose to close the returned
   handle afterwards. If INLEN is -1, we read as much as we can :) */
int mc_doublepopen (int inhandle, int inlen, pid_t *the_pid, char *command, ...)
{
    int pipe0 [2], pipe1 [2], std_sav [2];
#define MAXARGS 16
	int argno;
	char *args[MAXARGS];
	char buffer [8192];
	int i;
	va_list ap;

    pid_t pid;

	// Create the pipes
	if(_pipe(pipe0, 8192, O_BINARY | O_NOINHERIT) == -1)
   	    exit (1);
	if(_pipe(pipe1, 8192, O_BINARY | O_NOINHERIT) == -1)
   	    exit (1);
	// Duplicate stdin/stdout handles (next line will close original)
	std_sav[0] = _dup(_fileno(stdin));
	std_sav[1] = _dup(_fileno(stdout));
	// Duplicate read end of pipe0 to stdin handle
	if(_dup2(pipe0[0], _fileno(stdin)) != 0)
   	    exit (1);
	// Duplicate write end of pipe1 to stdout handle
	if(_dup2(pipe1[1], _fileno(stdout)) != 0)
   	    exit (1);
	// Close original read end of pipe0
	close(pipe0[0]);
	// Close original write end of pipe1
	close(pipe1[1]);

	va_start (ap, command);
	argno = 0;
	while ((args[argno++] = va_arg(ap, char *)) != NULL)
		if (argno == (MAXARGS - 1)) {
		args[argno] = NULL;
		break;
	}
	va_end (ap);
	// Spawn process
	pid = spawnvp(P_NOWAIT,command, args);// argv[1], (const char* const*)&argv[1]);
	if(!pid)
   	    exit (1);
	// Duplicate copy of original stdin back into stdin
	if(_dup2(std_sav[0], _fileno(stdin)) != 0)
   	    exit (1);
	// Duplicate copy of original stdout back into stdout
	if(_dup2(std_sav[1], _fileno(stdout)) != 0)
   	    exit (1);
	// Close duplicate copy of original stdout  and stdin    
	close(std_sav[0]);
	close(std_sav[1]);


	while ((i = _read (inhandle, buffer,
							 (inlen == -1 || inlen > 8192) 
							 ? 8192 : inlen)) > 0) {
		write (pipe0 [1], buffer, i);
		if (inlen != -1) {
			inlen -= i;
			if (!inlen)
				break;
		}
	}
	close (pipe0 [1]);
	*the_pid = pid;
    return pipe1 [0];

}

int mc_doublepclose (int pipe, pid_t pid)
{
    int status = 0;
    
    close (pipe);
    _cwait ( &status, pid, 0);
    return status;	
}

/*hacks to get it compile, remove these after vfs works */

/*hacks to get it compile, remove these after vfs works */
#ifndef USE_VFS
char *vfs_get_current_dir (void)
{
	return NULL;
}

int vfs_current_is_extfs (void)
{
	return 0;
}

int vfs_file_is_ftp (char *filename)
{
	return 0;
}

int mc_utime (char *path, void *times)
{
	return 0;
}


void extfs_run (char *file)
{
	return;
}
#endif

char *
get_default_editor (void)
{
   return "notepad.exe";
}

int
errno_dir_not_empty (int err)
{
    if (err == ENOTEMPTY || err == EEXIST || err == EACCES)
      return 1;
    return 0;
}

/* The MC library directory is by default the directory where mc.exe
   is situated. It is possible to specify this directory via MCHOME
   environment variable */
char *
get_mc_lib_dir ()
{
    char *cur;
    char *mchome = getenv("MCHOME");

    if (mchome && *mchome)
	return mchome;
    mchome = malloc(MC_MAXPATHLEN);
    GetModuleFileName(NULL, mchome, MC_MAXPATHLEN);
    for (cur = mchome + strlen(mchome); \
	(cur > mchome) && (*cur != PATH_SEP); cur--);
    *cur = 0;
    cur = strdup(mchome);
    free(mchome);
    if (!cur || !*cur) {
	free(cur);
        return "C:\\MC";
    }
    return cur;
}
int get_user_rights (struct stat *buf)
{
    return 2;
}
void init_groups (void)
{
}
void delete_groups (void)
{
}
