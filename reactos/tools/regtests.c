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
#include <ctype.h>
#ifndef WIN32
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
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
static char *umstubfile;
static char *kmstubfile;

char* convert_path(char* origpath)
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

static void write_line(char *line)
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

static void change_extension(char *filenamebuffer, char *filename, char *newextension)
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

/*
 * filename - name of file to make registrations for
 * regtype  - type of registration (0 = prototype, 1 = call, 2 = makefile)
 */
void register_test(char *filename, int type)
{
  char ext[100];
  char testname[100];
  char call[100];
  char regtest[100];
  char filenamebuffer[MAX_PATH];
  int i, j;

  strcpy(testname, filename);

  i = strlen(testname);
  while (i > 0 && testname[i] != '.')
    {
      i--;
    }
  if (i > 0)
    {
      memset(ext, 0, sizeof(ext));
      strncpy(&ext[0], &testname[i], strlen(&testname[i]));

      if ((strncmp(ext, ".c", 2) != 0) && (strncmp(ext, ".C", 2) != 0))
        {
          return;
        }

      testname[i] = 0;
    }
  else
    {
      return;
    }

  // Make a capital first letter and make all other letters lower case
  testname[0] = toupper(testname[0]);
  if (!((testname[0] >= 'A' && testname[0] <= 'Z') ||
    (testname[0] >= '0' && testname[0] <= '9')))
    {
      testname[0] = '_';
    }
  j = 1;
  while (j < strlen(testname))
    {
      testname[j] = tolower(testname[j]);
      if (!((testname[j] >= 'a' && testname[j] <= 'z') ||
        (testname[j] >= '0' && testname[j] <= '9')))
        {
          testname[j] = '_';
        }
      j++;
    }

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
is_file_changed(char *filename, char *content)
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
write_file_if_changed(char *filename, char *content)
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

static char KMSTUB[] =
  "/* This file is autogenerated. */\n"
  "\n"
  "#include <roskrnl.h>\n"
  "#include <../kmregtests/kmregtests.h>\n"
  "\n"
  "typedef int (*TestRoutine)(int Command, char *Buffer);\n"
  "\n"
  "extern void RegisterTests();\n"
  "\n"
  "static PDEVICE_OBJECT KMRegTestsDeviceObject = NULL;\n"
  "static PFILE_OBJECT KMRegTestsFileObject = NULL;\n"
  "\n"
  "void AddTest(TestRoutine Routine)\n"
  "{\n"
  "  PDEVICE_OBJECT DeviceObject;\n"
  "  UNICODE_STRING DriverName;\n"
  "  IO_STATUS_BLOCK IoStatus;\n"
  "  NTSTATUS Status;\n"
  "  KEVENT Event;\n"
  "  PIRP Irp;\n"
  "\n"
  "  if (KMRegTestsDeviceObject == NULL)\n"
  "    {\n"
  "      RtlInitUnicodeString(&DriverName, L\"\\\\Device\\\\KMRegTests\");\n"
  "	     Status = IoGetDeviceObjectPointer(&DriverName, FILE_WRITE_ATTRIBUTES,\n"
  "	       &KMRegTestsFileObject, &KMRegTestsDeviceObject);\n"
  "	     if (!NT_SUCCESS(Status)) return;\n"
  "	   }\n"
  "  KeInitializeEvent(&Event, NotificationEvent, FALSE);\n"
  "  Irp = IoBuildDeviceIoControlRequest(IOCTL_KMREGTESTS_REGISTER,\n"
  "	   KMRegTestsDeviceObject, &Routine, sizeof(TestRoutine), NULL, 0, FALSE, &Event, &IoStatus);\n"
  "  Status = IoCallDriver(KMRegTestsDeviceObject, Irp);\n"
  "}\n"
  "\n"
  "void PrepareTests()\n"
  "{\n"
  "  RegisterTests();\n"
  "}\n";

static char UMSTUB[] =
  "/* This file is autogenerated. */\n"
  "\n"
  "#include <windows.h>\n"
  "#define NTOS_MODE_USER\n"
  "#include <ntos.h>\n"
  "#include \"regtests.h\"\n"
  "\n"
  "PVOID\n"
  "AllocateMemory(ULONG Size)\n"
  "{\n"
  "  return (PVOID) RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);\n"
  "}\n"
  "\n"
  "VOID\n"
  "FreeMemory(PVOID Base)\n"
  "{\n"
  "  RtlFreeHeap(RtlGetProcessHeap(), 0, Base);\n"
  "}\n"
  "\n"
  "/* This function will be called several times */\n"
  "void PrepareTests()\n"
  "{\n"
  "  static int testsRegistered = 0;\n"
  "  if (testsRegistered == 0)\n"
  "    {\n"
  "	     HANDLE hEvent;\n"
  "	     hEvent = OpenEventW(\n"
  "        EVENT_ALL_ACCESS,\n"
  "        FALSE,\n"
  "        L\"WinRegTests\");\n"
  "	     if (hEvent != NULL)\n"
  "	       {\n"
  "		     SetEvent(hEvent);\n"
  "	         CloseHandle(hEvent);\n"
  "	         testsRegistered = 1;\n"
  "          InitializeTests();\n"
  "          RegisterTests();\n"
  "          PerformTests();\n"
  "        }\n"
  "    }\n"
  "}\n";

static char HELP[] =
  "REGTESTS path file makefile [-u umstubfile] [-k kmstubfile]\n"
  "\n"
  "  path        Path to files\n"
  "  file        Registration file to create\n"
  "  makefile    Makefile to create\n"
  "  umstubfile  Optional stub for running tests internal to a user-mode module\n"
  "  kmstubfile  Optional stub for running tests internal to a kernel-mode module\n";

int main(int argc, char **argv)
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

  umstubfile = NULL;
  kmstubfile = NULL;
  for (i = 4; i < argc; i++)
    {
	  if (argv[i][0] == '-')
	    {
	      if (argv[i][1] == 'u')
		    {
              umstubfile = convert_path(argv[++i]);
              if (umstubfile[0] == 0)
                {
                  printf("Missing umstubfile\n");
                  return 1;
                }
		    }
		  else if (argv[i][1] == 'k')
		    {
              kmstubfile = convert_path(argv[++i]);
              if (kmstubfile[0] == 0)
                {
                  printf("Missing kmstubfile\n");
                  return 1;
                }
		    }
		  else
		    {
              printf("Unknown switch\n");
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

  /* User-mode stubfile */
  if (umstubfile != NULL)
    {
      if (write_file_if_changed(umstubfile, UMSTUB) != 0)
        {
          perror("Cannot create output user-mode stubfile");
          return 1;
        }
    }

  /* Kernel-mode stubfile */
  if (kmstubfile != NULL)
    {
      if (write_file_if_changed(kmstubfile, KMSTUB) != 0)
        {
          perror("Cannot create output kernel-mode stubfile");
          return 1;
        }
    }

  printf("Successfully generated regression test registrations.\n");

  return 0;
}
