/*
 * Generate a file with test registrations from a list
 * of files in a directory.
 * Casper S. Hornstrup <chorns@users.sourceforge.net>
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#include <io.h>
#include <dos.h>
#else
#include <sys/io.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef WIN32
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif

static FILE *out;
static char *path;
static char *file;
static char *makefile;
static char *exestubfile;

static char*
convert_path(char* origpath)
{
   char* newpath;
   int i;
   
	 /* for no good reason, i'm having trouble getting gcc to link strdup */
   //newpath = strdup(origpath);
	 newpath = malloc(strlen(origpath)+1);
	 strcpy(newpath, origpath);
   
   i = 0;
   while (newpath[i] != 0)
     {
#ifndef WIN32
	if (newpath[i] == '\\')
	  {
	     newpath[i] = '/';
	  }
#else
#ifdef WIN32
	if (newpath[i] == '/')
	  {
	     newpath[i] = '\\';
	  }
#endif	
#endif	
	i++;
     }
   return(newpath);
}

static void
write_line(char *line)
{
  int n_out;
  char buf[200];

  memset(buf, 0, sizeof(buf));
  strcpy(buf, line);
  /* Terminate the line */
  buf[strlen(buf)] = '\r';
  buf[strlen(buf)] = '\n';

  n_out = fwrite(&buf[0], 1, strlen(buf), out);
}

static void
change_extension(char *filenamebuffer,
  char *filename,
  char *newextension)
{
  char *ptr;

  if (newextension == NULL)
    {
      strcpy(filenamebuffer, filename);
      return;
    }

  ptr = strrchr(filename, '.');
  if (ptr != NULL)
    {
      strncpy(filenamebuffer, filename, ptr - filename);
      filenamebuffer[ptr - filename] = 0;
      strcat(filenamebuffer, newextension);
    }
  else
    {
      strcpy(filenamebuffer, filename);
      strcat(filenamebuffer, newextension);
    }
}

static void
get_test_name(char *filename,
  char *testname)
{
  int i;

  strcpy(testname, filename);

  i = strlen(testname);
  while (i > 0 && testname[i] != '.')
    {
      i--;
    }
  if (i > 0)
    {
      testname[i] = 0;
    }

  /* Make a capital first letter and make all other letters lower case */
  testname[0] = toupper(testname[0]);
  if (!((testname[0] >= 'A' && testname[0] <= 'Z') ||
    (testname[0] >= '0' && testname[0] <= '9')))
    {
      testname[0] = '_';
    }
  i = 1;
  while (i < strlen(testname))
    {
      testname[i] = tolower(testname[i]);
      if (!((testname[i] >= 'a' && testname[i] <= 'z') ||
        (testname[i] >= '0' && testname[i] <= '9')))
        {
          testname[i] = '_';
        }
      i++;
    }
}

/*
 * filename - name of file to make registrations for
 * type     - type of registration (0 = prototype, 1 = call, 2 = makefile)
 */
static void
register_test(char *filename,
  int type)
{
  char ext[100];
  char testname[100];
  char call[100];
  char regtest[100];
  char filenamebuffer[MAX_PATH];
  int i;

  i = strlen(filename);
  while (i > 0 && filename[i] != '.')
    {
      i--;
    }
  if (i > 0)
    {
      memset(ext, 0, sizeof(ext));
      strncpy(&ext[0], &filename[i], strlen(&filename[i]));

      if (strcasecmp(ext, ".c") != 0)
        {
          return;
        }
    }
  else
    {
      return;
    }

  memset(testname, 0, sizeof(testname));
  get_test_name(filename, testname);

  if (type == 0)
    {
      sprintf(regtest, "extern int %sTest(int Command, char *Buffer);", testname);
      write_line(regtest);
    }
  else if (type == 1)
    {
      sprintf(call, "%sTest", testname);
      sprintf(regtest, "  AddTest((TestRoutine)%s);", call);
      write_line(regtest);
    }
  else if (type == 2)
    {
	  change_extension(filenamebuffer, filename, ".o");
      sprintf(regtest, "%s \\", filenamebuffer);
      write_line(regtest);
    }
}

#ifdef WIN32

/* Win32 version */

static void
make_file_list (int type)
{
  struct _finddata_t f;
  int findhandle;
  char searchbuf[MAX_PATH];

  strcpy(searchbuf, path);
  strcat(searchbuf, "*.*");
  findhandle =_findfirst(searchbuf, &f);
  if (findhandle != -1)
    {
      do
      	{
      	  if (f.attrib & _A_SUBDIR)
      	    {
              /* Skip subdirectories */
              continue;
      	    }

          register_test(f.name, type);
      	}
      while (_findnext(findhandle, &f) == 0);
      _findclose(findhandle);
    }
}

#else

/* Linux version */
static void
make_file_list (int type)
{
  DIR *dirp;
  struct dirent *entry;
  struct stat stbuf;
  char buf[MAX_PATH];

#ifdef HAVE_D_TYPE
  dirp = opendir(path);
  if (dirp != NULL)
    {
      while ((entry = readdir(dirp)) != NULL)
        {
          if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; // skip self and parent

      	  if (entry->d_type == DT_REG) // normal file
      	    {
              // Check for an absolute path
              if (path[0] == DIR_SEPARATOR_CHAR)
                {
                  strcpy(buf, path);
                  strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, entry->d_name);
                }
              else
                {
                  getcwd(buf, sizeof(buf));
                  strcat(buf, DIR_SEPARATOR_STRING);
                  strcat(buf, path);
                  strcat(buf, entry->d_name);
                }

  	      if (stat(buf, &stbuf) == -1)
            {
              printf("Can't access '%s' (%s)\n", buf, strerror(errno));
              return;
            }

          if (S_ISDIR(stbuf.st_mode))
      	    {
              /* Skip subdirectories */
              continue;
      	    }

          register_test(entry->d_name, type);
         }
      }
      closedir(dirp);
    }
  else
    {
      printf("Can't open %s\n", path);
      return;
    }

#else

  dirp = opendir(path);
  if (dirp != NULL)
    {
      while ((entry = readdir(dirp)) != NULL)
      	{
          if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; // skip self and parent

          // Check for an absolute path
          if (path[0] == DIR_SEPARATOR_CHAR)
            {
              strcpy(buf, path);
              strcat(buf, DIR_SEPARATOR_STRING);
              strcat(buf, entry->d_name);
            }
          else
            {
              getcwd(buf, sizeof(buf));
              strcat(buf, DIR_SEPARATOR_STRING);
              strcat(buf, path);
              strcat(buf, entry->d_name);
            }

          if (stat(buf, &stbuf) == -1)
            {
              printf("Can't access '%s' (%s)\n", buf, strerror(errno));
              return;
            }

          if (S_ISDIR(stbuf.st_mode))
      	    {
              /* Skip subdirectories */
              continue;
      	    }

          register_test(entry->d_name, type);
        }
      closedir(dirp);
    }
  else
    {
      printf("Can't open %s\n", path);
      return;
    }

#endif
}

#endif

static int
is_file_changed(char *filename,
  char *content)
{
  FILE *file;
  int size;
  int n;
  char *filecontent;

  file = fopen(filename, "rb");
  if (file == NULL)
    {
    	return 1;
     }

  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, 0, SEEK_SET);
  if (size <= 0)
    {
      fclose(file);
	  return 1;
    }
  filecontent = malloc(size);
  if (filecontent == NULL)
    {
      fclose(file);
	  return 1;
    }

  n = fread(filecontent, 1, size, file);

  if (n != strlen(content))
    {
	  free(filecontent);
      fclose(file);
	  return 1;
    }

  if (strcmp(content, filecontent) != 0)
    {
	  free(filecontent);
      fclose(file);
	  return 1;
    }

  free(filecontent);

  fclose(file);

  return 0;
}

static int
write_file_if_changed(char *filename,
  char *content)
{
  FILE *file;
  int n;

  if (is_file_changed(filename, content) == 0)
    {
	  return 0;
    }

  file = fopen(filename, "wb");
  if (file == NULL)
    {
    	return 1;
     }

  n = fwrite(content, 1, strlen(content), file);

  fclose(file);

  return 0;
}

static char EXESTUB[] =
  "/* This file is autogenerated. */\n"
  "\n"
  "#include <windows.h>\n"
  "#include \"regtests.h\"\n"
  "\n"
  "void\n"
  "ConsoleWrite(char *Buffer)\n"
  "{\n"
  "  printf(Buffer);\n"
  "}\n"
  "\n"
  "int\n"
  "mainCRTStartup(HANDLE hInstance,\n"
  "  HANDLE hPrevInstance,\n"
  "  LPSTR lpszCmdParam,\n"
  "  int nCmdShow)\n"
  "{\n"
  "  InitializeTests();\n"
  "  RegisterTests();\n"
  "  SetupOnce();\n"
  "  PerformTests(ConsoleWrite, NULL);\n"
  "  _ExitProcess(0);\n"
  "  return 0;\n"
  "}\n";

static char STUBS_HEADER[] =
  "/* This file is autogenerated. */\n"
  "passthrough:\n"
  "  call _FrameworkGetHook@4\n"
  "  test %eax, %eax\n"
  "  je .return\n"
  "  jmp *%eax\n"
  ".return:\n"
  "  /* This will most likely corrupt the stack */\n"
  "  ret\n"
  "\n";

static char HOOKS_HEADER[] =
  "/* This file is autogenerated. */\n"
  "#include <windows.h>\n"
  "#include \"regtests.h\"\n"
  "\n"
  "API_DESCRIPTION ExternalDependencies[] =\n"
  "{\n";

static char HOOKS_FOOTER[] =
  "};\n"
  "\n"
  "#define ExternalDependencyCount %d\n"
  "ULONG MaxExternalDependency = ExternalDependencyCount - 1;\n";

static char HELP[] =
  "REGTESTS path file makefile [-e exestubfile]\n"
  "REGTESTS -s stublistfile stubsfile hooksfile\n"
  "\n"
  "  path         Path to files\n"
  "  file         Registration file to create\n"
  "  makefile     Makefile to create\n"
  "  exestubfile  Optional stub for running tests in the build environment\n"
  "  stublistfile File with descriptions of stubs\n"
  "  stubsfile    File with stubs to create\n"
  "  hooksfile    File with hooks to create\n";

#define INPUT_BUFFER_SIZE 255

void
write_stubs_header(FILE * out)
{
  fputs(STUBS_HEADER, out);
}

void
write_hooks_header(FILE * out)
{
  fputs(HOOKS_HEADER, out);
}

void
write_hooks_footer(FILE *hooks_out, unsigned long nr_stubs)
{
  fprintf(hooks_out, HOOKS_FOOTER, nr_stubs);
}

char *
get_symbolname(char *decoratedname)
{
  char buf[300];

  if (decoratedname[0] == '@')
    return strdup(decoratedname);
  strcpy(buf, "_");
  strcat(buf, decoratedname);
  return strdup(buf);
}

char *
get_undecorated_name(char *buf,
  char *decoratedname)
{
  int start = 0;
  int end = 0;

  while (start < strlen(decoratedname) && decoratedname[start] == '@')
    {
      start++;
    }
  strcpy(buf, &decoratedname[start]);
  end = strlen(buf) - 1;
  while (end > 0 && isdigit(buf[end]))
    {
      end--;
    }
  if (buf[end] == '@')
    {
      buf[end] = 0;
    }
  return buf;
}

char *
get_forwarded_export(char *forwardedexport)
{
  char buf[300];

  if (forwardedexport == NULL)
    {
      strcpy(buf, "NULL");
    }
  else
    {
      sprintf(buf, "\"%s\"", forwardedexport);
    }
  return strdup(buf);
}

void
write_stub(FILE *stubs_out, FILE *hooks_out, char *dllname,
  char *decoratedname_and_forward, unsigned int stub_index)
{
  char buf[300];
  char *p;
  char *decoratedname = NULL;
  char *forwardedexport = NULL;
  char *symbolname = NULL;

  p = strtok(decoratedname_and_forward, "=");
  if (p != NULL)
    {
      decoratedname = p;

      p = strtok(NULL, "=");
      forwardedexport = p;
    }
  else
    {
      decoratedname = decoratedname_and_forward;
      forwardedexport = decoratedname_and_forward;
    }

  symbolname = get_symbolname(decoratedname);
  fprintf(stubs_out, ".globl %s\n", symbolname);
  fprintf(stubs_out, "%s:\n", symbolname);
  free(symbolname);
  fprintf(stubs_out, "  pushl $%d\n", stub_index);
  fprintf(stubs_out, "  jmp passthrough\n");
  fprintf(stubs_out, "\n");
  forwardedexport = get_forwarded_export(forwardedexport);
  fprintf(hooks_out, "  {\"%s\", \"%s\", %s, NULL, NULL},\n",
    dllname,
    get_undecorated_name(buf, decoratedname),
    forwardedexport);
  free(forwardedexport);
}

void
create_stubs_and_hooks(
  FILE *in,
  FILE *stubs_out,
  FILE *hooks_out)
{
    char line[INPUT_BUFFER_SIZE];
    char *s, *start;
    char *dllname;
    char *decoratedname_and_forward;
    int stub_index;
  
    write_stubs_header(stubs_out);
    
    write_hooks_header(hooks_out);
    
    /*
     * Scan the database. The database is a text file; each
     * line is a record, which contains data for one stub.
     * Each record has two columns:
     *
     * DLLNAME (e.g. ntdll.dll)
     * DECORATED NAME (e.g. NtCreateProcess@32, @InterlockedIncrement@4 or printf)
     */
    stub_index = 0; /* First stub has index zero */
    
    for (
	;
	/* Go on until EOF or read zero bytes */
	((!feof(in)) && (fgets(line, sizeof line, in) != NULL));
	/* Next stub index */
	)
    {
	/*
	 * Ignore leading blanks
	 */
	for( start = line; *start && isspace(*start); start++ );
	    
	/*
	 * Strip comments, eols
	 */
	for( s = start; *s && !strchr("#\n\r", *s); s++ );
	
	*s = '\0';

	/*
	 * Remove trailing blanks.  Backup off the char that ended our
	 * run before.
	 */
	for( s--; s > start && isspace(*s); s-- ) *s = '\0';

	/*
	 * Skip empty lines 
	 */
	if (s > start)
	{
	    /* Extract the DLL name */
	    dllname = (char *) strtok(start, " \t");
	    if (dllname != NULL && strlen(dllname) > 0)
	    {
		/*
		 * Extract the decorated function name and possibly forwarded export.
		 * Format:
		 *   decoratedname=forwardedexport (no DLL name)
		 */
		decoratedname_and_forward = (char *) strtok(NULL, " \t");
		/* Extract the argument count */
		
		/* Something went wrong finding the separator ... 
		 * print an error and bail. */
		if( !decoratedname_and_forward ) {
		    fprintf
			( stderr, 
			  "Could not find separator between decorated "
			  "function name and dll name.\n"
			  "Format entries as <dllname> <import>\n"
			  "Legal comments start with #\n");
		    exit(1);
		}
		
		write_stub(stubs_out, hooks_out, dllname, decoratedname_and_forward, stub_index);
		stub_index++;
	    }
	}
    }
    
    write_hooks_footer(hooks_out, stub_index);
}

int run_stubs(int argc,
  char **argv)
{
  FILE *in;
  FILE *stubs_out;
  FILE *hooks_out;
  
	in = fopen(argv[2], "rb");
	if (in == NULL)
  	{
  		perror("Failed to open stub description input file");
  		return 1;
  	}

	stubs_out = fopen(argv[3], "wb");
	if (stubs_out == NULL)
	{
		perror("Failed to open stubs output file");
		return 1;
	}

	hooks_out = fopen(argv[4], "wb");
	if (hooks_out == NULL)
	{
		perror("Failed to open hooks output file");
		return 1;
	}

  create_stubs_and_hooks(in, stubs_out, hooks_out);

  fclose(stubs_out);
  fclose(hooks_out);

  return 0;
}

int run_registrations(int argc,
  char **argv)
{
  char buf[MAX_PATH];
  int i;

  if (argc < 4)
  {
    puts(HELP);
    return 1;
  }

  strcpy(buf, convert_path(argv[1]));
  if (buf[strlen(buf)] != DIR_SEPARATOR_CHAR)
    {
      int i = strlen(buf);
      buf[strlen(buf)] = DIR_SEPARATOR_CHAR;
      buf[i + 1] = 0;
    }
  path = buf;
  if (path[0] == 0)
    {
      printf("Missing path\n");
      return 1;
    }

  file = convert_path(argv[2]);
  if (file[0] == 0)
    {
      printf("Missing file\n");
      return 1;
    }

  makefile = convert_path(argv[3]);
  if (makefile[0] == 0)
    {
      printf("Missing makefile\n");
      return 1;
    }

  exestubfile = NULL;
  for (i = 4; i < argc; i++)
    {
	  if (argv[i][0] == '-')
	    {
        if (argv[i][1] == 'e')
  		    {
            exestubfile = convert_path(argv[++i]);
            if (exestubfile[0] == 0)
              {
                printf("Missing exestubfile\n");
                return 1;
              }
  		    }
  		  else
  		    {
                printf("Unknown switch -%c\n", argv[i][1]);
                return 1;
  		    }
	    }
	 }


  /* Registration file */
  out = fopen(file, "wb");
  if (out == NULL)
    {
    	perror("Cannot create output file");
    	return 1;
     }

  write_line("/* This file is autogenerated. */");
  write_line("");
  write_line("typedef int (*TestRoutine)(int Command, char *Buffer);");
  write_line("");

  make_file_list(0);

  write_line("");
  write_line("extern void AddTest(TestRoutine Routine);");
  write_line("");
  write_line("void RegisterTests()");
  write_line("{");

  make_file_list(1);

  write_line("}");

  fclose(out);


  /* Makefile */
  out = fopen(makefile, "wb");
  if (out == NULL)
    {
    	perror("Cannot create output makefile");
    	return 1;
     }

  write_line("# This file is autogenerated.");
  write_line("");
  write_line("TESTS = \\");

  make_file_list(2);

  write_line("");

  fclose(out);

  /* Executable stubfile */
  if (exestubfile != NULL)
    {
      if (write_file_if_changed(exestubfile, EXESTUB) != 0)
       {
          perror("Cannot create output executable stubfile");
          return 1;
        }
    }

  return 0;
}

int main(int argc,
  char **argv)
{
  if (argc < 2)
  {
    puts(HELP);
    return 1;
  }

  if (strlen(argv[1]) > 1 && argv[1][0] == '-' && argv[1][1] == 's')
    {
      return run_stubs(argc, argv);
    }
  else
    {
      return run_registrations(argc, argv);
    }
}
