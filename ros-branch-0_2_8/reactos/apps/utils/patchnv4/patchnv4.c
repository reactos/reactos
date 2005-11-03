/* $Id$
 *
 * Patch the NVidia miniport driver to work with ReactOS
 *
 * Should become obsolete
 */

#include <stdio.h>
#include <stdlib.h>

struct Patch
{
  long Offset;
  unsigned char ExpectedValue;
  unsigned char NewValue;
};

static struct Patch Patches[ ] =
{
  { 0x1EBA9, 0x30, 0x3C },
  { 0x1EBAA, 0xC0, 0xF0 },
  { 0x1EC0B, 0x04, 0x01 },
  { 0x1EC67, 0x30, 0x3C },
  { 0x1EC68, 0xC0, 0xF0 }
};

int
main(int argc, char *argv[])
{
  static char OriginalName[] = "nv4_mini.sys";
  static char TempName[] = "nv4_mini.tmp";
  static char BackupName[] = "nv4_mini.sys.orig";
  FILE *File;
  unsigned char *Buffer;
  long Size;
  unsigned n;

  /* Read the whole file in memory */
  File = fopen(OriginalName, "rb");
  if (NULL == File)
    {
      perror("Unable to open original file");
      exit(1);
    }
  if (fseek(File, 0, SEEK_END))
    {
      perror("Unable to determine file length");
      fclose(File);
      exit(1);
    }
  Size = ftell(File);
  if (-1 == Size)
    {
      perror("Unable to determine file length");
      fclose(File);
      exit(1);
    }
  Buffer = malloc(Size);
  if (NULL == Buffer)
    {
      perror("Can't allocate buffer");
      fclose(File);
      exit(1);
    }
  rewind(File);
  if (Size != fread(Buffer, 1, Size, File))
    {
      perror("Error reading from original file");
      free(Buffer);
      fclose(File);
      exit(1);
    }
  fclose(File);

  /* Patch the file */
  for (n = 0; n < sizeof(Patches) / sizeof(struct Patch); n++)
    {
      if (Buffer[Patches[n].Offset] != Patches[n].ExpectedValue)
        {
          fprintf(stderr, "Expected value 0x%02x at offset 0x%lx but found 0x%02x\n",
                  Patches[n].ExpectedValue, Patches[n].Offset,
                  Buffer[Patches[n].Offset]);
          free(Buffer);
          exit(1);
        }
      Buffer[Patches[n].Offset] = Patches[n].NewValue;
    }

  /* Write the new file */
  File = fopen(TempName, "wb");
  if (NULL == File)
    {
      perror("Unable to open output file");
      free(Buffer);
      exit(1);
    }
  if (Size != fwrite(Buffer, 1, Size, File))
    {
      perror("Error writing to output file");
      fclose(File);
      remove(TempName);
      free(Buffer);
      exit(1);
    }
  fclose(File);
  free(Buffer);

  /* Rename the original file, removing an existing backup */
  remove(BackupName);
  if (0 != rename(OriginalName, BackupName))
    {
      perror("Failed to rename original file");
      remove(TempName);
      exit(1);
    }

  /* Rename the new file */
  if (0 != rename(TempName, OriginalName))
    {
      perror("Failed to rename new file");
      remove(TempName);
      rename(BackupName, OriginalName);
      exit(1);
    }

  return 0;
}
