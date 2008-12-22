#include <windows.h>
#include <conio.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/** DEFINES *******************************************************************/

#define PATCH_BUFFER_SIZE           4096    /* Maximum size of a patch */
#define PATCH_BUFFER_MAGIC          "\xde\xad\xbe\xef MaGiC MaRk "
#define SIZEOF_PATCH_BUFFER_MAGIC   (sizeof (PATCH_BUFFER_MAGIC) - 1)

/** TYPES *********************************************************************/

typedef struct _PatchedByte
{
   int            offset;    /*!< File offset of the patched byte. */
   unsigned char  expected;  /*!< Expected (original) value of the byte. */
   unsigned char  patched;   /*!< Patched (new) value for the byte. */
} PatchedByte;

typedef struct _PatchedFile
{
   const char  *name;        /*!< Name of the file to be patched. */
   int          fileSize;    /*!< Size of the file in bytes. */
   int          patchCount;  /*!< Number of patches for the file. */
   PatchedByte *patches;     /*!< Patches for the file. */
} PatchedFile;

typedef struct _Patch
{
   const char   *name;       /*!< Name of the patch. */
   int           fileCount;  /*!< Number of files in the patch. */
   PatchedFile  *files;      /*!< Files for the patch. */
} Patch;

/** FUNCTION PROTOTYPES *******************************************************/

static void printUsage();

/** GLOBALS *******************************************************************/

static Patch m_patch = { NULL, 0, NULL };
static int m_argc = 0;
static char **m_argv = NULL;

/* patch buffer where we put the patch info into */
static char m_patchBuffer[SIZEOF_PATCH_BUFFER_MAGIC + PATCH_BUFFER_SIZE] =
   PATCH_BUFFER_MAGIC;

/** HELPER FUNCTIONS **********************************************************/

static void *
loadFile(const char *fileName, int *fileSize_)
{
   FILE *f;
   struct stat sb;
   int fileSize;
   void *p;

   /* Open the file */
   f = fopen(fileName, "rb");
   if (f == NULL)
   {
      printf("Couldn't open file %s for reading!\n", fileName);
      return NULL;
   }

   /* Get file size */
   if (fstat(fileno(f), &sb) < 0)
   {
      fclose(f);
      printf("Couldn't get size of file %s!\n", fileName);
      return NULL;
   }
   fileSize = sb.st_size;

   /* Load file */
   p = malloc(fileSize);
   if (p == NULL)
   {
      fclose(f);
      printf("Couldn't allocate %d bytes for file %s!\n", fileSize, fileName);
      return NULL;
   }

   if (fread(p, fileSize, 1, f) != 1)
   {
      fclose(f);
      free(p);
      printf("Couldn't read file %s into memory!\n", fileName);
      return NULL;
   }

   /* Close file */
   fclose(f);

   *fileSize_ = fileSize;
   return p;
}


static int
saveFile(const char *fileName, void *file, int fileSize)
{
   FILE *f;

   /* Open the file */
   f = fopen(fileName, "wb");
   if (f == NULL)
   {
      printf("Couldn't open file %s for writing!\n", fileName);
      return -1;
   }

   /* Write file */
   if (fwrite(file, fileSize, 1, f) != 1)
   {
      fclose(f);
      printf("Couldn't write file %s!\n", fileName);
      return -1;
   }

   /* Close file */
   fclose(f);
   return 0;
}


static int
compareFiles(
   PatchedFile *patchedFile,
   const char *originalFileName)
{
   const char *patchedFileName = patchedFile->name;
   unsigned char *origChunk, *patchedChunk;
   int origSize, patchedSize, i, patchCount;
   PatchedByte *patches = NULL;
   int patchesArrayCount = 0;

   /* Load both files */
   origChunk = loadFile(originalFileName, &origSize);
   if (origChunk == NULL)
      return -1;
   patchedChunk = loadFile(patchedFileName, &patchedSize);
   if (patchedChunk == NULL)
   {
      free(origChunk);
      return -1;
   }
   if (origSize != patchedSize)
   {
      free(origChunk);
      free(patchedChunk);
      printf("File size of %s and %s differs (%d != %d)\n",
             originalFileName, patchedFileName,
             origSize, patchedSize);
      return -1;
   }

   /* Compare the files and record any differences */
   printf("Comparing %s to %s", originalFileName, patchedFileName);
   for (i = 0, patchCount = 0; i < origSize; i++)
   {
      if (origChunk[i] != patchedChunk[i])
      {
         patchCount++;

         /* Resize patches array if needed */
         if (patchesArrayCount < patchCount)
         {
            PatchedByte *newPatches;
            newPatches = realloc(patches, patchCount * sizeof (PatchedByte));
            if (newPatches == NULL)
            {
              if (patches != NULL)
                free(patches);
              free(origChunk);
              free(patchedChunk);
              printf("\nOut of memory (tried to allocated %d bytes)\n",
                     patchCount * sizeof (PatchedByte));
              return -1;
            }
            patches = newPatches;
         }

         /* Fill in patch info */
         patches[patchCount - 1].offset = i;
         patches[patchCount - 1].expected = origChunk[i];
         patches[patchCount - 1].patched = patchedChunk[i];
      }
      if ((i % (origSize / 40)) == 0)
         printf(".");
   }
   printf(" %d changed bytes found.\n", patchCount);

   /* Unload the files */
   free(origChunk);
   free(patchedChunk);

   /* Save patch info */
   patchedFile->fileSize = patchedSize;
   patchedFile->patchCount = patchCount;
   patchedFile->patches = patches;

   return 0;
}


static int
outputPatch(const char *outputFileName)
{
   char *patchExe, *patchBuffer = NULL;
   int i, size, patchExeSize, patchSize, stringSize, stringOffset, patchOffset;
   Patch *patch;
   PatchedFile *files;

   printf("Putting patch into %s...\n", outputFileName);

   /* Calculate size of the patch */
   patchSize = sizeof (Patch) + sizeof (PatchedFile) * m_patch.fileCount;
   stringSize = strlen(m_patch.name) + 1;
   for (i = 0; i < m_patch.fileCount; i++)
   {
      stringSize += strlen(m_patch.files[i].name) + 1;
      patchSize += sizeof (PatchedByte) * m_patch.files[i].patchCount;
   }
   if ((stringSize + patchSize) > PATCH_BUFFER_SIZE)
   {
      printf("Patch is too big - %d bytes maximum, %d bytes needed\n",
             PATCH_BUFFER_SIZE, stringSize + patchSize);
      return -1;
   }

   /* Load patch.exe file into memory... */
   patchExe = loadFile(m_argv[0], &patchExeSize);
   if (patchExe == NULL)
   {
      return -1;
   }

   /* Try to find the magic mark for the patch buffer */
   for (i = 0; i < (patchExeSize - SIZEOF_PATCH_BUFFER_MAGIC); i++)
   {
      if (memcmp(patchExe + i, m_patchBuffer, SIZEOF_PATCH_BUFFER_MAGIC) == 0)
      {
         patchBuffer = patchExe + i + SIZEOF_PATCH_BUFFER_MAGIC;

         break;
      }
   }
   if (!(i < (patchExeSize - SIZEOF_PATCH_BUFFER_MAGIC)))
   {
      free(patchExe);
      printf("Couldn't find patch buffer magic in file %s - this shouldn't happen!!!\n", m_argv[0]);
      return -1;
   }

   /* Pack patch together and replace string pointers by offsets */
   patch = (Patch *)patchBuffer;
   files = (PatchedFile *)(patchBuffer + sizeof (Patch));
   patchOffset = sizeof (Patch) + sizeof (PatchedFile) * m_patch.fileCount;
   stringOffset = patchSize;

   patch->fileCount = m_patch.fileCount;
   patch->files = (PatchedFile *)sizeof (Patch);

   patch->name = (const char *)stringOffset;
   strcpy(patchBuffer + stringOffset, m_patch.name);
   stringOffset += strlen(m_patch.name) + 1;

   for (i = 0; i < m_patch.fileCount; i++)
   {
      files[i].fileSize = m_patch.files[i].fileSize;
      files[i].patchCount = m_patch.files[i].patchCount;

      files[i].name = (const char *)stringOffset;
      strcpy(patchBuffer + stringOffset, m_patch.files[i].name);
      stringOffset += strlen(m_patch.files[i].name) + 1;

      size = files[i].patchCount * sizeof (PatchedByte);
      files[i].patches = (PatchedByte *)patchOffset;
      memcpy(patchBuffer + patchOffset, m_patch.files[i].patches, size);
      patchOffset += size;
   }
   size = patchSize + stringSize;
   memset(patchBuffer + size, 0, PATCH_BUFFER_SIZE - size);

   /* Save file */
   if (saveFile(outputFileName, patchExe, patchExeSize) < 0)
   {
      free(patchExe);
      return -1;
   }
   free(patchExe);

   printf("Patch saved!\n");
   return 0;
}


static int
loadPatch()
{
   char *p;
   Patch *patch;
   int i;

   p = m_patchBuffer + SIZEOF_PATCH_BUFFER_MAGIC;
   patch = (Patch *)p;

   if (patch->name == NULL)
   {
      return -1;
   }

   m_patch.name = p + (int)patch->name;
   m_patch.fileCount = patch->fileCount;
   m_patch.files = (PatchedFile *)(p + (int)patch->files);

   for (i = 0; i < m_patch.fileCount; i++)
   {
      m_patch.files[i].name = p + (int)m_patch.files[i].name;
      m_patch.files[i].patches = (PatchedByte *)(p + (int)m_patch.files[i].patches);
   }

   printf("Patch %s loaded...\n", m_patch.name);
   return 0;
}


/** MAIN FUNCTIONS ************************************************************/

static int
createPatch()
{
   int i, status;
   const char *outputFileName;

   /* Check argument count */
   if (m_argc < 6 || (m_argc % 2) != 0)
   {
      printUsage();
      return -1;
   }

   outputFileName = m_argv[3];
   m_patch.name = m_argv[2];

   /* Allocate PatchedFiles array */
   m_patch.fileCount = (m_argc - 4) / 2;
   m_patch.files = malloc(m_patch.fileCount * sizeof (PatchedFile));
   if (m_patch.files == NULL)
   {
      printf("Out of memory!\n");
      return -1;
   }
   memset(m_patch.files, 0, m_patch.fileCount * sizeof (PatchedFile));

   /* Compare original to patched files and fill m_patch.files array */
   for (i = 0; i < m_patch.fileCount; i++)
   {
      m_patch.files[i].name = m_argv[4 + (i * 2) + 1];
      status = compareFiles(m_patch.files + i, m_argv[4 + (i * 2) + 0]);
      if (status < 0)
      {
         for (i = 0; i < m_patch.fileCount; i++)
         {
            if (m_patch.files[i].patches != NULL)
               free(m_patch.files[i].patches);
         }
         free(m_patch.files);
         m_patch.files = NULL;
         m_patch.fileCount = 0;
         return status;
      }
   }

   /* Output patch */
   return outputPatch(outputFileName);
}


static int
applyPatch()
{
   int c, i, j, fileSize, makeBackup;
   unsigned char *file;
   char *p;
   const char *fileName;
   char buffer[MAX_PATH];


   if (m_argc > 1 && strcmp(m_argv[1], "-d") != 0)
   {
      printUsage();
      return -1;
   }

   /* Load patch */
   if (loadPatch() < 0)
   {
      printf("This executable doesn't contain a patch, use -c to create one.\n");
      return -1;
   }

   if (m_argc > 1)
   {
      /* Dump patch */
      printf("Patch name: %s\n", m_patch.name);
      printf("File count: %d\n", m_patch.fileCount);
      for (i = 0; i < m_patch.fileCount; i++)
      {
         printf("----------------------\n"
                "File name:   %s\n"
                "File size:   %d bytes\n",
                m_patch.files[i].name, m_patch.files[i].fileSize);
         printf("Patch count: %d\n", m_patch.files[i].patchCount);
         for (j = 0; j < m_patch.files[i].patchCount; j++)
         {
            printf("  Offset 0x%x   0x%02x -> 0x%02x\n",
                   m_patch.files[i].patches[j].offset,
                   m_patch.files[i].patches[j].expected,
                   m_patch.files[i].patches[j].patched);
         }
      }
   }
   else
   {
      /* Apply patch */
      printf("Applying patch...\n");
      for (i = 0; i < m_patch.fileCount; i++)
      {
         /* Load original file */
         fileName = m_patch.files[i].name;
applyPatch_retry_file:
         file = loadFile(fileName, &fileSize);
         if (file == NULL)
         {
            printf("File %s not found! ", fileName);
applyPatch_file_open_error:
            printf("(S)kip, (R)etry, (A)bort, (M)anually enter filename");
            do
            {
               c = _getch();
            }
            while (c != 's' && c != 'r' && c != 'a' && c != 'm');
            printf("\n");
            if (c == 's')
            {
               continue;
            }
            else if (c == 'r')
            {
               goto applyPatch_retry_file;
            }
            else if (c == 'a')
            {
               return 0;
            }
            else if (c == 'm')
            {
               if (fgets(buffer, sizeof (buffer), stdin) == NULL)
               {
                  printf("fgets() failed!\n");
                  return -1;
               }
               p = strchr(buffer, '\r');
               if (p != NULL)
                  *p = '\0';
               p = strchr(buffer, '\n');
               if (p != NULL)
                  *p = '\0';

               fileName = buffer;
               goto applyPatch_retry_file;
            }
         }

         /* Check file size */
         if (fileSize != m_patch.files[i].fileSize)
         {
            free(file);
            printf("File %s has unexpected filesize of %d bytes (%d bytes expected)\n",
                   fileName, fileSize, m_patch.files[i].fileSize);
            if (fileName != m_patch.files[i].name) /* manually entered filename */
            {
               goto applyPatch_file_open_error;
            }
            return -1;
         }

         /* Ask for backup */
         printf("Do you want to make a backup of %s? (Y)es, (N)o, (A)bort", fileName);
         do
         {
            c = _getch();
         }
         while (c != 'y' && c != 'n' && c != 'a');
         printf("\n");
         if (c == 'y')
         {
            char buffer[MAX_PATH];
            _snprintf(buffer, MAX_PATH, "%s.bak", fileName);
            buffer[MAX_PATH-1] = '\0';
            makeBackup = 1;
            if (access(buffer, 0) >= 0) /* file exists */
            {
               printf("File %s already exists, overwrite? (Y)es, (N)o, (A)bort", buffer);
               do
               {
                  c = _getch();
               }
               while (c != 'y' && c != 'n' && c != 'a');
               printf("\n");
               if (c == 'n')
                  makeBackup = 0;
               else if (c == 'a')
               {
                  free(file);
                  return 0;
               }
            }
            if (makeBackup && saveFile(buffer, file, fileSize) < 0)
            {
               free(file);
               return -1;
            }
         }
         else if (c == 'a')
         {
            free(file);
            return 0;
         }

         /* Patch file */
         for (j = 0; j < m_patch.files[i].patchCount; j++)
         {
            int offset = m_patch.files[i].patches[j].offset;
            if (file[offset] != m_patch.files[i].patches[j].expected)
            {
               printf("Unexpected value in file %s at offset 0x%x: expected = 0x%02x, found = 0x%02x\n",
                      fileName, offset, m_patch.files[i].patches[j].expected, file[offset]);
               free(file);
               return -1;
            }
            file[offset] = m_patch.files[i].patches[j].patched;
         }

         /* Save file */
         if (saveFile(fileName, file, fileSize) < 0)
         {
            free(file);
            return -1;
         }
         free(file);
      }

      printf("Patch applied sucessfully!\n");
   }

   return 0;
}


static void
printUsage()
{
   printf("Usage:\n"
          "%s -c     - Create patch\n"
          "%s -d     - Dump patch\n"
          "%s        - Apply patch\n"
          "\n"
          "A patch can be created like this:\n"
          "%s -c \"patch name\" output.exe file1.orig file1.patched[ file2.orig file2.patched[ ...]]\n",
          m_argv[0], m_argv[0], m_argv[0], m_argv[0]);
}


int
main(
   int argc,
   char *argv[])
{
   m_argc = argc;
   m_argv = argv;

   if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
   {
      printUsage();
      return 0;
   }
   else if (argc >= 2 && argv[1][0] == '-')
   {
      if (strcmp(argv[1], "-c") == 0)
      {
         return createPatch();
      }
      else if (strcmp(argv[1], "-d") == 0)
      {
         return applyPatch();
      }
      else
      {
         printf("Unknown option: %s\n"
                "Use -h for help.\n",
                argv[1]);
         return -1;
      }
   }

   return applyPatch();
}

