/* Various utilities - OS/2 versions
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

#define INCL_DOS
#define INCL_PM
#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#define INCL_DOSDEVICES   /* Device values */
#define INCL_DOSDATETIME
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <signal.h>		/* my_system */
#include <limits.h>		/* INT_MAX */
#include <sys/time.h>		/* select: timeout */
#include <sys/param.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <process.h>
#include "../src/fs.h"
#include "../src/util.h"
#include "../src/dialog.h"

#ifndef ENOTEMPTY
#define ENOTEMPTY ERROR_DIR_NOT_EMPTY
#endif

char *
get_owner (int uid)
{
    return "none";
}

char *
get_group (int gid)
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
void 
open_error_pipe (void)
{
   return;
}

void 
close_error_pipe (int error, char *text)
{
   return;
}

void 
check_error_pipe (void)
{
    char error[MAX_PIPE_SIZE];
    int len = 0;
    if (old_error >= 0){
	while (len < MAX_PIPE_SIZE)
	{
	    int rvalue;

	    rvalue = read (error_pipe[0], error + len, 1);
	    len ++;
	    if (rvalue <= 0)
		break;
	}
	error[len] = 0;
	close (error_pipe[0]);
    }
    if (len > 0)
        message (0, " Warning ", error);
}


static int 
StartWindowsProg (char *name, SHORT type)
{
#if 0 /* FIXME: PM DDL's should be loaded (or not loaded) at run time */
   PROGDETAILS  pDetails;

   memset(&pDetails, 0, sizeof(PROGDETAILS)) ;
   pDetails.Length = sizeof(pDetails);
   pDetails.pszExecutable = name;      /* program name */
   pDetails.pszStartupDir = NULL;   /* default directory for new app. */
   pDetails.pszParameters = NULL;   /* command line */
   pDetails.progt.fbVisible = SHE_VISIBLE ;
   pDetails.pszEnvironment = NULL;

   switch (type) {
   case 0:
      /* Win Standard */
      pDetails.progt.progc = PROG_31_ENHSEAMLESSCOMMON ;
      break;
   case 1:
      /* Win 3.1 Protect */
      pDetails.progt.progc = PROG_31_ENHSEAMLESSCOMMON ;
      break;
   case 2:
      /* Win 3.1 Enh. Protect */
      pDetails.progt.progc = PROG_31_ENHSEAMLESSCOMMON ;
      break;
   default:
      pDetails.progt.progc = PROG_31_ENHSEAMLESSCOMMON ;
     break;
   }
   WinStartApp(NULLHANDLE, 
               &pDetails, 
               NULL, 
               NULL, 
               SAF_INSTALLEDCMDLINE|SAF_STARTCHILDAPP) ;
#endif
   return 0;
}


static int 
os2_system (int as_shell_command, const char *shell, const char *command, char *parm);

/* 
  as_shell_command = 1: If a program is started during input line, CTRL-O
                        or RETURN 
                   = 0: F3, F4
*/
int 
my_system (int as_shell_command, const char *shell, const char *command)
{
   char *sh;            /* This is the shell -- always! */
   char *cmd;           /* This is the command (only the command) */
   char *parm;          /* This is the parameter (can be more than one) */
   register int length, i;
   char temp[4096];     /* That's enough! */

   sh = get_default_shell();
   if (strcmp(sh, shell)) {
      /* 
         Not equal  -- That means: shell is the program and command is the
         parameter 
      */
      cmd  = (char *) shell;
      parm = (char *) command;
   } else {
      /* look into the command and take out the program */
      if (command) {
         strcpy(temp, command);
         length = strlen(command);
         for (i=length-1; i>=0; i--) {
            if (command[i] == ' ') {
               temp[i] = (char) 0;
               length--;
            } else 
                break;
         }
         if (i==-1) {
            /* only blanks */
            return -1;
         }
         if (parm = strchr(temp, (char) ' ')) {
            *parm = (char) 0;
            parm++;
         }
         cmd  = (char *) temp;
      } else {
         /* command is NULL */
         cmd = parm = NULL;
      }
   }
   return os2_system (as_shell_command, sh, cmd, parm);
}

static int 
ux_startp (const char *shell, const char *command, const char *parm) 
{
    if (parm) {
         spawnlp (P_WAIT, 
              (char *) shell, 
              (char *) shell, 
              "/c", 
              (char *) command, 
              (char *) parm,
              (char *) 0);
    } else {
         spawnlp (P_WAIT, 
              (char *) shell, 
              (char *) shell, 
              "/c", 
              (char *) command, 
              (char *) 0);
    }
    return 0;
}


static int 
os2_system (int as_shell_command, const char *shell, const char *command, char *parm)
{
   register int i, j;
   ULONG        AppType = 0;                    /* Application type flags (returned) */
   APIRET       rc = NO_ERROR;                  /* Return Code */
   char         pathValue[5] = "PATH";          /* For DosSearchPath */
   UCHAR        searchResult[MC_MAXPATHLEN * 2 + 1];     /* For DosSearchPath */
   
   char         *cmdString;
   char         *postFix[3];
   char         *line;
   /* ------------------------------------------------------- */
   STARTDATA    StartData;
   CHAR         ObjBuf[100];
   ULONG        SessionID;
   PID          pid;

   if (command == NULL) {
      /* .ado: just start a shell, we don't need the parameter */
      spawnl (P_WAIT, 
              (char *) shell, 
              (char *) shell, 
              (char *) command, (char *) 0);  
      return 0;
   }

   memset(&StartData, 0, sizeof(StartData)) ;
   StartData.Length             = sizeof(StartData);    
   StartData.Related            = SSF_RELATED_CHILD;
   StartData.FgBg               = SSF_FGBG_BACK;
   StartData.TraceOpt           = SSF_TRACEOPT_NONE;   
   StartData.PgmTitle           = NULL;                
   StartData.TermQ              = NULL;                 
   StartData.InheritOpt         = SSF_INHERTOPT_PARENT;
   StartData.IconFile           = 0;              
   StartData.PgmHandle          = 0;             
   StartData.PgmControl         = SSF_CONTROL_VISIBLE ; 
   StartData.ObjectBuffer       = ObjBuf;          
   StartData.ObjectBuffLen      = 100;            
   StartData.PgmInputs          = parm;

   postFix[0] = ".exe";
   postFix[1] = ".cmd";
   postFix[2] = ".bat";

   i = strlen(command);
   if (command[i-1] == ' ') {
      /* The user has used ALT-RETURN */
      i--;
   }
   cmdString = (char *) malloc(i+1);
   for (j=0; j<i; j++) {
      cmdString[j] = command[j];
   }
   cmdString[j] = (char) 0;

   if ((i < 5) || ((i > 4) && (cmdString[i-4]) != '.')) {
      /* without Extension */
      line = (char *) malloc(i+5);
      rc = 1;
      for (i=0; (i<3 && rc); i++) {
         /* Search for the file */
         strcpy(line, cmdString);
         strcat(line, postFix[i]);
         rc = DosSearchPath((SEARCH_IGNORENETERRS | SEARCH_ENVIRONMENT | SEARCH_CUR_DIRECTORY),
                            (PSZ) pathValue,
                            line,
                            searchResult,
                            sizeof(searchResult));
      }
      free (line);
   } else {         
      /* Just search */
      rc = DosSearchPath((SEARCH_IGNORENETERRS | SEARCH_ENVIRONMENT | SEARCH_CUR_DIRECTORY),
                         (PSZ) pathValue,
                         cmdString,
                         searchResult,
                         sizeof(searchResult));
   }
   free(cmdString);
   if (rc != 0) {
      /* Internal command or the program was written with absolut path */
      return ux_startp(shell, command, parm);
   }

   /* Application to be started */
   StartData.PgmName            = searchResult;
   StartData.Environment        = NULL;
   rc = DosQueryAppType(searchResult, &AppType);
   if (rc == NO_ERROR) {
      StartData.SessionType = PROG_WINDOWABLEVIO;
      if ((AppType & 0x00000007) == FAPPTYP_WINDOWAPI) {
         /* Window API */
         StartData.SessionType = PROG_PM;
         return DosStartSession(&StartData, &SessionID, &pid);
      }
      if ((AppType & 0x00000007) == FAPPTYP_WINDOWCOMPAT) {
         /* Window compat */
         return ux_startp(shell, command, parm);
      }
      if (AppType & 0x0000ffff & FAPPTYP_DOS) {
         /* PC/DOS Format */
        StartData.SessionType = PROG_WINDOWEDVDM;
        return DosStartSession(&StartData, &SessionID, &pid);
      }
      if (AppType & 0x0000ffff & FAPPTYP_WINDOWSREAL) {
         /* Windows real mode app */
        return StartWindowsProg(searchResult, 0);
      }
      if (AppType & 0x0000ffff & FAPPTYP_WINDOWSPROT) {
         /* Windows Protect mode app*/
        return StartWindowsProg(searchResult, 1);
      }
      if (AppType & 0x0000ffff & FAPPTYP_WINDOWSPROT31) {
         /* Windows 3.1 Protect mode app*/
        return StartWindowsProg(searchResult, 2);
      }
      rc = DosStartSession(&StartData, &SessionID, &pid) ;
   } else {
      /* It's not a known exe type or it's a CMD/BAT file */
      i = strlen(searchResult);
      if ((toupper(searchResult[--i]) == 'T') && 
          (toupper(searchResult[--i]) == 'A') &&  
          (toupper(searchResult[--i]) == 'B') &&  
          (searchResult[--i] == '.')   ) {
        StartData.SessionType = PROG_WINDOWEDVDM;
        rc = DosStartSession(&StartData, &SessionID, &pid) ;
      } else {
         rc = ux_startp (shell, command, parm);
      }
   }
   return rc;
}

char *tilde_expand (char *directory)
{
    return strdup (directory);
}


/* Canonicalize path, and return a new path. Do everything in situ.
   The new path differs from path in:
	Multiple BACKSLASHs are collapsed to a single BACKSLASH.
	Leading `./'s and trailing `/.'s are removed.
	Trailing BACKSLASHs are removed.
	Non-leading `../'s and trailing `..'s are handled by removing
	portions of the path. */
char *
canonicalize_pathname (char *path)
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

        /* Handle multiple BACKSLASHs in a row. */
        while (path[i] == PATH_SEP)
	    i++;

        if ((start + 1) != i) {
	    strcpy (path + start + 1, path + i);
	    i = start + 1;
	}

        /* Handle backquoted BACKSLASH. */
/*         if (start > 0 && path[start - 1] == '\\')
	    continue; */

        /* Check for trailing BACKSLASH. */
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
	        if (!strncmp (path + start + 1, "..\\", 3))
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


void 
my_statfs (struct my_statfs *myfs_stats, char *path)
{
    PFSALLOCATE pBuf;
    PFSINFO     pFsInfo;
    ULONG       lghBuf;

    ULONG       diskNum = 0;
    ULONG       logical = 0;

    UCHAR       szDeviceName[3] = "A:";
    PBYTE       pszFSDName      = NULL;  /* pointer to FS name            */
    APIRET      rc              = NO_ERROR; /* Return code                */
    BYTE        fsqBuffer[sizeof(FSQBUFFER2) + (3 * CCHMAXPATH)] = {0};
    ULONG       cbBuffer   = sizeof(fsqBuffer);        /* Buffer length) */
    PFSQBUFFER2 pfsqBuffer = (PFSQBUFFER2) fsqBuffer;

    int i, len = 0;

    /* ------------------------------------------------------------------ */

    lghBuf = sizeof(FSALLOCATE);
    pBuf = (PFSALLOCATE) malloc(lghBuf);

    /* Get the free number of Bytes */
    rc = DosQueryFSInfo(0L, FSIL_ALLOC, (PVOID) pBuf, lghBuf);
    /* KBytes available */
    myfs_stats->avail = pBuf->cSectorUnit * pBuf->cUnitAvail * pBuf->cbSector / 1024;
    /* KBytes total */
    myfs_stats->total = pBuf->cSectorUnit * pBuf->cUnit * pBuf->cbSector / 1024; 
    myfs_stats->nfree = pBuf->cUnitAvail;
    myfs_stats->nodes = pBuf->cbSector;

    lghBuf  = sizeof(FSINFO);
    pFsInfo = (PFSINFO) malloc(lghBuf);
    rc      = DosQueryFSInfo(0L, 
                             FSIL_VOLSER, 
                             (PVOID) pFsInfo, 
                             lghBuf);
    /* Get name */
    myfs_stats->device = strdup(pFsInfo->vol.szVolLabel);    /* Label of the Disk */

    /* Get the current disk for DosQueryFSAttach */
    rc = DosQueryCurrentDisk(&diskNum, &logical);

    szDeviceName[0] = (UCHAR) (diskNum + (ULONG) 'A' - 1);
    /* Now get the type of the disk */
    rc = DosQueryFSAttach(szDeviceName, 
                          0L, 
                          FSAIL_QUERYNAME, 
                          pfsqBuffer, 
                          &cbBuffer);

    pszFSDName = pfsqBuffer->szName + pfsqBuffer->cbName + 1;
    myfs_stats->mpoint = strdup(pszFSDName);    /* FAT, HPFS ... */

    myfs_stats->type = pBuf->idFileSystem;
    /* What is about 3 ?*/
    if (myfs_stats->type == 0) {
       myfs_stats->typename = (char *) malloc(11);
       strcpy(myfs_stats->typename, "Local Disk");
    } else {
       myfs_stats->typename = (char *) malloc(13);
       strcpy(myfs_stats->typename, "Other Device");
    }

    free(pBuf);
    free(pFsInfo);
}

int 
gettimeofday (struct timeval* tvp, void *p)
{
   DATETIME     pdt = {0};
   if (p != NULL)		/* what is "p"? */
	return 0;	
	
    /* Since MC only calls this func from get_random_hint we return 
     * some value, not exactly the "correct" one
     */
    DosGetDateTime(&pdt);
    tvp->tv_usec = (pdt.hours * 60 + pdt.minutes) * 60 + pdt.seconds;
    /* Number of milliseconds since Windows started */
    tvp->tv_sec = tvp->tv_usec * 1000 + pdt.hundredths * 10;
    return 0;
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
      }
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

int 
getuid ()	      
{
    return 0;
}

int 
getgid ()	      
{
    return 0;
}

int 
readlink (char* path, char* buf, int size)
{
    return -1;
}

int 
symlink (char *n1, char *n2)
{
    return -1;
}

int 
link (char *p1, char *p2)
{
    return -1;
}

int 
chown (char *path, int owner, int group)
{
    return -1;
}

int 
mknod (char *path, int mode, int dev)
{
    return -1;
}

void 
init_uid_gid_cache (void)
{
    return;
}

int 
mc_doublepopen (int inhandle, int inlen, pid_t *the_pid, char *command, ...)
{
	return 0;
}

int 
mc_doublepclose (int pipe, pid_t pid)
{
	return 0;
}

/*hacks to get it compile, remove these after vfs works */
char *
vfs_get_current_dir (void)
{
	return NULL;
}

int 
vfs_current_is_extfs (void)
{
	return 0;
}

int 
vfs_file_is_ftp (char *filename)
{
	return 0;
}

int
mc_utime (char *path, void *times)
{
	return 0;
}


void
extfs_run (char *file)
{
   return;
}

int
geteuid(void)
{
   return 0;
}


int
mc_chdir(char *pathname)
{
   APIRET       ret;
   register int lgh = strlen(pathname);

   /* Set the current drive */
   if (lgh == 0) {
      return -1;
   } else {
      /* First set the default drive */
      if (lgh > 1) {
         if (pathname[1] == ':') {
             ret = DosSetDefaultDisk(toupper(pathname[0]) - 'A' + 1); 
         }
      }
      /* After that, set the current dir! */
      ret = DosSetCurrentDir(pathname);
   }
   return ret;
}
 
int
mc_chmod(char *pathName, int unxmode)
{
   /* OS/2 does not need S_REG */
   int os2Mode = unxmode & 0x0FFF;
   return chmod(pathName, os2Mode);
}

static int
conv_os2_unx_rc(int os2rc)
{
   int errCode;
   switch (os2rc) {
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
      case ERROR_FILENAME_EXCED_RANGE:
         errCode = ENOENT;
         break;
      case ERROR_NOT_DOS_DISK:
      case ERROR_SHARING_VIOLATION:
      case ERROR_SHARING_BUFFER_EXCEEDED:
      case ERROR_ACCESS_DENIED:
         errCode = EACCES;
         break;
      case ERROR_INVALID_PARAMETER:
         errCode = EINVAL;
         break;
      default: 
         errCode = EINVAL;
        break;
   }
   return errCode;
}

int
mc_open (char *file, int flags, int pmode)
{
    return open(file, (flags | O_BINARY), pmode);
}

int
mc_unlink(char *pathName)
{
   /* Use OS/2 API to delete a file, if the file is set as read-only, 
      the file will be deleted without asking the user! */
   APIRET       rc;
   rc = DosDelete(pathName);
   if (!rc) {
      return 0;
   }
   if (rc == ERROR_ACCESS_DENIED) {
      chmod(pathName, (S_IREAD|S_IWRITE));
      rc = DosDelete(pathName);
      if (rc) {
         errno = conv_os2_unx_rc(rc) ;
         return -1;
      } else {
         return 0;
      }
   } else {
      errno = conv_os2_unx_rc(rc) ;
      return -1;
   }
}

char *
get_default_editor (void)
{
	char *tmp;
	APIRET  rc;
	char    pathValue[5] = "PATH";
	UCHAR   searchResult[MC_MAXPATHLEN + 1];
	
        /* EPM is not always be installed */
	rc = DosSearchPath((SEARCH_IGNORENETERRS | SEARCH_ENVIRONMENT | SEARCH_CUR_DIRECTORY),
			   (PSZ) pathValue,
			   "EPM.EXE",
			   searchResult,
			   sizeof(searchResult));
	if (rc != 0) {
                /* The system editor is always there */
		return strdup("e.exe");
	} else {
                /* Let it be searched from my_system */
		return strdup("epm.exe");
	}
}

/* get_default_shell
   Get the default shell for the current hardware platform
   TODO: Get the value of %OS2_SHELL% or %SHELL%: which one?
*/
char *
get_default_shell()
{
    return getenv ("COMSPEC");
}

int
errno_dir_not_empty (int err)
{
    if (err == ENOTEMPTY)
	return 1;
    return 0;
}

/* The MC library directory is by default the directory where mc.exe
   is situated. It is recommended to specify this directory via MCHOME
   environment variable, otherwise you will be unable to rename mc.exe */
char *
get_mc_lib_dir ()
{
    HMODULE mc_hm;
    int rc;
    char *cur = NULL;
    char *mchome = getenv("MCHOME");

    if (mchome && *mchome)
	return mchome;
    mchome = malloc(MC_MAXPATHLEN);
    rc = DosQueryModuleHandle ("MC.EXE", &mc_hm);
    if (!rc)
	rc = DosQueryModuleName (mc_hm, MC_MAXPATHLEN, mchome);
    if (!rc)
    {
	for (cur = mchome + strlen(mchome); \
	    (cur > mchome) && (*cur != PATH_SEP); cur--);
	*cur = 0;
	cur = strdup(mchome);
	free(mchome);
    }
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
