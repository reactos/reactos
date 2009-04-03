/*
 *  REN.C - rename internal command.
 *
 *
 *  History:
 *    
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    18-Dec-1998 (Eric Kohl)
 *        Added support for quoted long file names with spaces.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    17-Oct-2001 (Eric Kohl)
 *        Implemented basic rename code.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *	  25-Nov-2008 (Victor Martinez) <vicmarcal@hotmail.com> Patch dedicated to Myrjala because her comprenhension and love :D
 *		  Fixing following Bugs: 
 *	           -Wrong behavior with wildcards when Source and Destiny are Paths(FIXED).
 *			   -Wrong general behavior (MSDN:"Rename cant move files between subdirectories")(FIXED)
 *			   -Wrong behavior when renaming without path in destiny:(i.e) "ren C:\text\as.txt list.txt" it moves as.txt and then rename it(FIXED)
 *				(MSDN: If there is a Path in Source and no Path in Destiny, then Destiny Path is Source Path,because never Ren has to be used to move.)
 *			   -Implemented checkings if SourcePath and DestinyPath are differents.
 *	     
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_RENAME

enum
{
  REN_ATTRIBUTES = 0x001,   /* /A : not implemented */
  REN_ERROR      = 0x002,   /* /E */
  REN_NOTHING    = 0x004,   /* /N */
  REN_PROMPT     = 0x008,   /* /P : not implemented */
  REN_QUIET      = 0x010,   /* /Q */
  REN_SUBDIR     = 0x020,   /* /S */
  REN_TOTAL      = 0x040,   /* /T */
};


/*
 *  file rename internal command.
 *
 */
INT cmd_rename (LPTSTR param)
{
  LPTSTR *arg = NULL;
  INT args = 0;
  INT nSlash = 0;
  INT nEvalArgs = 0; /* nunber of evaluated arguments */
  DWORD dwFlags = 0;
  DWORD dwFiles = 0; /* number of renamedd files */
  INT i;

 
  LPTSTR srcPattern = NULL; /* Source Argument*/
  TCHAR srcPath[MAX_PATH]; /*Source Path Directories*/
  LPTSTR srcFILE = NULL;  /*Contains the files name(s)*/
  TCHAR srcFinal[MAX_PATH]; 
  
 
  LPTSTR dstPattern = NULL; /*Destiny Argument*/
  TCHAR dstPath[MAX_PATH]; /*Source Path Directories*/
  LPTSTR dstFILE = NULL; /*Contains the files name(s)*/

  TCHAR dstLast[MAX_PATH]; /*It saves the File name after unmasked with wildcarts*/
  TCHAR dstFinal[MAX_PATH]; /*It saves the Final destiny Path*/

  BOOL bDstWildcard = FALSE;
  BOOL bPath = FALSE;
  
 
  
 
  
  
  LPTSTR p,q,r;

  HANDLE hFile;
  WIN32_FIND_DATA f;
 /*If the PARAM=/? then show the help*/
  if (!_tcsncmp(param, _T("/?"), 2))
  {
	  
	
    ConOutResPaging(TRUE,STRING_REN_HELP1);
    return 0;
  }

  nErrorLevel = 0;

  /* Split the argument list.Args will be saved in arg vector*/
  arg = split(param, &args, FALSE);
 	
  if (args < 2)
    {
      if (!(dwFlags & REN_ERROR))
	error_req_param_missing();
      freep(arg);
      return 1;
    }

  /* Read options */
  for (i = 0; i < args; i++)
    {
	/* Lets check if we have a special option choosen and set the flag(s)*/
      if (*arg[i] == _T('/'))
		{
			if (_tcslen(arg[i]) >= 2)
			 {
				 switch (_totupper(arg[i][1]))
					{
						 case _T('E'):
						 dwFlags |= REN_ERROR;
						 break;

						 case _T('N'):
						dwFlags |= REN_NOTHING;
						 break;
			
						case _T('P'):
						dwFlags |= REN_PROMPT;
						break;

						case _T('Q'):
					    dwFlags |= REN_QUIET;
						break;

						case _T('S'):
						dwFlags |= REN_SUBDIR;
						break;

						case _T('T'):
						dwFlags |= REN_TOTAL;
						break;
					}
				}
		nEvalArgs++;//Save the number of the options.
		}
    }

  /* keep quiet within batch files */
  if (bc != NULL)
    dwFlags |= REN_QUIET;

  /* there are only options on the command line --> error!!! */
  if (args < nEvalArgs + 2)
    {
      if (!(dwFlags & REN_ERROR))
		error_req_param_missing();
		freep(arg);
		return 1;
    }
  
  
  /* Get destination pattern and source pattern*/
  for (i = 0; i < args; i++)
    {
      if (*arg[i] == _T('/'))//We have find an Option.Jump it.
		continue;
      dstPattern = arg[i]; //we save the Last argument as dstPattern
	  srcPattern = arg[i-1];
	  
    }
  
 
 
  

  if (_tcschr(srcPattern, _T('\\')))  //Checking if the Source (srcPattern) is a Path to the file
	{	
		
		bPath= TRUE;

        //Splitting srcPath and srcFile.

		srcFILE = _tcschr(srcPattern, _T('\\'));
		nSlash++;
		while(_tcschr(srcFILE, _T('\\')))
			{
			srcFILE++;
			if(*srcFILE==_T('\\')) nSlash++ ;
			if(!_tcschr(srcFILE, _T('\\'))) break;
			}
		_tcsncpy(srcPath,srcPattern,_tcslen(srcPattern)-_tcslen(srcFILE));
		
	
			
		if(_tcschr(dstPattern, _T('\\'))) //Checking if the Destiny (dstPattern)is also a Path.And splitting dstPattern in dstPath and srcPath.
			{  
				dstFILE = _tcschr(dstPattern, _T('\\'));
				nSlash=0;
				while(_tcschr(dstFILE, _T('\\')))
						{
						dstFILE++;
						if(*dstFILE==_T('\\')) nSlash++ ;
						if(!_tcschr(dstFILE, _T('\\'))) break;
						}
				_tcsncpy(dstPath,dstPattern,_tcslen(dstPattern)-_tcslen(dstFILE));
				
				if((_tcslen(dstPath)!=_tcslen(srcPath))||(_tcsncmp(srcPath,dstPath,_tcslen(srcPath))!=0)) //If it has a Path,then MUST be equal than srcPath
						{
						error_syntax(dstPath);
						freep(arg);
						return 1;
						}
			}else	{ //If Destiny hasnt a Path,then (MSDN says) srcPath is its Path. 
					
					_tcscpy(dstPath,srcPath);
					
					dstFILE=dstPattern;
					
					}

		
	
	}
  
  if (!_tcschr(srcPattern, _T('\\'))) //If srcPattern isnt a Path but a  name:
  { 
	srcFILE=srcPattern;
	if(_tcschr(dstPattern, _T('\\')))
				{		
						error_syntax(dstPattern);
						
						freep(arg);
						return 1;
				}else dstFILE=dstPattern;
  }
 
 //Checking Wildcards.
  if (_tcschr(dstFILE, _T('*')) || _tcschr(dstFILE, _T('?')))
		bDstWildcard = TRUE;
  
  
  
  TRACE("\n\nSourcePattern: %s SourcePath: %s SourceFile: %s", debugstr_aw(srcPattern),debugstr_aw(srcPath),debugstr_aw(srcFILE));
  TRACE("\n\nDestinationPattern: %s Destination Path:%s Destination File: %s\n", debugstr_aw(dstPattern),debugstr_aw(dstPath),debugstr_aw(dstFILE));
 
      hFile = FindFirstFile(srcPattern, &f);
	  
      if (hFile == INVALID_HANDLE_VALUE)
		{
		 if (!(dwFlags & REN_ERROR))
				error_file_not_found();
				
		}
      do
		{
	  /* ignore "." and ".." */
		 if (!_tcscmp (f.cFileName, _T(".")) ||
			  !_tcscmp (f.cFileName, _T("..")))
		 continue;

	  /* do not rename hidden or system files */
		 if (f.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
			continue;

	  /* do not rename directories when the destination pattern contains
	   * wildcards, unless option /S is used */
		  if ((f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			 && bDstWildcard
			 && !(dwFlags & REN_SUBDIR))
			continue;

		 TRACE("Found source name: %s\n", debugstr_aw(f.cFileName));
	  /* So here we have splitted the dstFILE and we have find a f.cFileName(thanks to srcPattern) 
	   * Now we have to use the mask (dstFILE) (which can have Wildcards) with f.cFileName to find destination file name(dstLast) */
		 p = f.cFileName;
		 q = dstFILE;
		 r = dstLast;
		 while(*q != 0)
			{
				if (*q == '*')
					{
						q++;
						while (*p != 0 && *p != *q)
							{
							*r = *p;
							p++;
							r++;
							}
					}
					else if (*q == '?')
								{
								q++;
								if (*p != 0)
									 {
									*r = *p;
									p++;
									r++;
									 }
								}
								else
									{
									*r = *q;
									if (*p != 0)
											p++;
									q++;
									r++;
									}
			}
		*r = 0;
		//Well we have splitted the Paths,so now we have to paste them again(if needed),thanks bPath.
		if(	bPath == TRUE)
			{	
				
				_tcscpy(srcFinal,srcPath);
				
				_tcscat(srcFinal,f.cFileName);
				 
				_tcscpy(dstFinal,dstPath);
				_tcscat(dstFinal,dstLast);
				 
								
		}else{
				_tcscpy(srcFinal,f.cFileName);
				_tcscpy(dstFinal,dstLast);

				}
		


		TRACE("DestinationPath: %s\n", debugstr_aw(dstFinal));
		
		
		if (!(dwFlags & REN_QUIET) && !(dwFlags & REN_TOTAL))
	  
			 ConOutPrintf(_T("%s -> %s\n"),srcFinal , dstFinal);

	  /* Rename the file */
		 if (!(dwFlags & REN_NOTHING))
		 {	 
				
				
	      
				if (MoveFile(srcFinal, dstFinal))
			{
			dwFiles++;
			}
			else
				{
				 if (!(dwFlags & REN_ERROR))
						{
						ConErrResPrintf(STRING_REN_ERROR1, GetLastError());
						}
				}
		 }
	  }
	   
      while (FindNextFile(hFile, &f));
//Closing and Printing errors.
	
      FindClose(hFile);
   

  if (!(dwFlags & REN_QUIET))
  {
    if (dwFiles == 1)
      ConOutResPrintf(STRING_REN_HELP2, dwFiles);
    else
      ConOutResPrintf(STRING_REN_HELP3, dwFiles);
  }
 
  freep(arg);
  
  return 0;
}

#endif

/* EOF */
