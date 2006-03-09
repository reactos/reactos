#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_CYLINDERS         80
#define ROOT_ENTRY_SIZE     32

#define SECTOR_SIZE         512
#define SECTORS_PER_CLUSTER 1
#define N_RESERVED          1
#define N_FATS              2
#define N_ROOT_ENTRIES      224
#define SECTORS_PER_DISK    (N_HEADS * N_CYLINDERS * SECTORS_PER_TRACK)
#define MEDIA_TYPE          0xf0
#define SECTORS_PER_FAT     9
#define SECTORS_PER_TRACK   18
#define N_HEADS             2
#define SIGNATURE           0x29        /* only MS? */
#define END_SIGNATURE       0xaa55


#define ATTR_READONLY 0x01
#define ATTR_HIDDEN   0x02
#define ATTR_SYSTEM   0x04
#define ATTR_VOLUME   0x08
#define ATTR_SUBDIR   0x10
#define ATTR_ARCHIVE  0x20
#define ATTR_RES1     0x40
#define ATTR_RES2     0x80


typedef unsigned char disk_sector_t[SECTOR_SIZE];

typedef struct boot_sector
{
  unsigned short jmp;
  unsigned char  nop;
  char           oem[8];
  unsigned short bytes_per_sector;
  unsigned char  sectors_per_cluster;
  unsigned short reserved_sectors;
  unsigned char  n_fats;
  unsigned short n_root_entries;
  unsigned short n_sectors;
  unsigned char  media_type;
  unsigned short sectors_per_fat;
  unsigned short sectors_per_track;
  unsigned short n_heads;
  unsigned long  hidden_sectors;
  unsigned long  huge_sectors;
  unsigned char  drive;
  unsigned char  reserved;
  unsigned char  signature;
  unsigned long  volume_id;
  char           volume_label[11];
  char           file_system[8];
  unsigned char  boot_code[SECTOR_SIZE - 62 - 2];
  unsigned short end_signature;
} __attribute__ ((packed)) boot_sector_t;


typedef struct root_entry
{
  char           name[8];
  char           extension[3];
  unsigned char  attribute;
  unsigned char  reserved[10];
  unsigned short time;
  unsigned short date;
  unsigned short cluster;
  unsigned long  size;
} __attribute ((packed)) root_entry_t;


disk_sector_t *new_image(char *bsfname)
{
  FILE *bsf;
  disk_sector_t *img;
  boot_sector_t boot_sec;
  root_entry_t *root;

  if ((bsf = fopen(bsfname, "rb")) == NULL)
  {
    printf("Boot sector image file %s not found!\n", bsfname);
    return NULL;
  }
  if (fread(&boot_sec, 1, SECTOR_SIZE, bsf) != SECTOR_SIZE)
  {
    printf("Unable to read boot sector image file %s!\n", bsfname);
    fclose(bsf);
    return NULL;
  }
  fclose(bsf);

  if ( (boot_sec.bytes_per_sector != SECTOR_SIZE) ||
       (boot_sec.sectors_per_cluster != SECTORS_PER_CLUSTER) ||
       (boot_sec.reserved_sectors != N_RESERVED) ||
       (boot_sec.n_fats != N_FATS) ||
       (boot_sec.n_root_entries != N_ROOT_ENTRIES) ||
       (boot_sec.n_sectors != SECTORS_PER_DISK) ||
       (boot_sec.media_type != MEDIA_TYPE) ||
       (boot_sec.sectors_per_fat != SECTORS_PER_FAT) ||
       (boot_sec.sectors_per_track != SECTORS_PER_TRACK) ||
       (boot_sec.n_heads != N_HEADS) ||
//       (boot_sec.signature != SIGNATURE) ||
       (boot_sec.end_signature != END_SIGNATURE) )
  {
    printf("Invalid boot sector in file %s\n", bsfname);
    return NULL;
  }

  if ((img = (disk_sector_t *)malloc(SECTOR_SIZE * SECTORS_PER_DISK)) == NULL)
  {
    printf("Not enough memory!\n");
    return NULL;
  }

  memset(img, 0, SECTOR_SIZE * SECTORS_PER_TRACK);
  memcpy(img, &boot_sec, SECTOR_SIZE);

  root = (root_entry_t *)img[N_RESERVED + N_FATS * SECTORS_PER_FAT];
  strncpy(root->name, "REACTOS       ", 11);
  root->attribute = ATTR_VOLUME;
  
  return img;
}


void create_root_entry(root_entry_t *root, char *fname,
                       unsigned short cluster, unsigned long size)
{
  int i, j;
  time_t t;
  struct tm *localt;

  i = 0;
  j = 0;
  while ((fname[j] != '\0') && (fname[j] != '.') && (i < 8))
  {
    root->name[i] = toupper(fname[j]);
    i++;
    j++;
  }
  while (i < 8)
  {
    root->name[i] = ' ';
    i++;
  }
  if (fname[j] == '.')
  {
    i = 0;
    j++;
    while ((fname[j] != '\0') && (i < 3))
    {
      root->extension[i] = toupper(fname[j]);
      i++;
      j++;
    }
    while (i < 3)
    {
      root->extension[i] = ' ';
      i++;
    }
  }
  else
  {
    i = 0;
    while (i < 3)
    {
      root->extension[i] = ' ';
      i++;
    }
  }

  root->attribute = ATTR_ARCHIVE;
  t = time(0);
  localt = localtime(&t);
  root->time = (((localt->tm_hour & 0x001f) << 11) |
                ((localt->tm_min & 0x003f) << 5) |
                ((localt->tm_sec / 2) & 0x001f));
  root->date = ((((localt->tm_year - 80) & 0x007f) << 9) |
                (((localt->tm_mon + 1) & 0x000f) << 5) |
                 (localt->tm_mday & 0x001f));
  root->cluster = cluster;
  root->size = size;
}


void update_fat(unsigned char *fat, int cl_start, int cl_end)
{
  int i, k;
  unsigned short *cl;

  for (i = cl_start; i < cl_end - 1; i++)
  {
    k = (i - 2) * 3 / 2;
    cl = ((unsigned short *)&fat[k]);
    if (i & 1)
    {
      *cl = (*cl & 0x000f) | (((i + 1) & 0x0fff) << 4);
    }
    else
    {
      *cl = (*cl & 0xf000) | ((i + 1) & 0x0fff);
    }
  }
  k = (i - 2) * 3 / 2;
  cl = ((unsigned short *)&fat[k]);
  if (i & 1)
  {
    *cl = (*cl & 0x000f) | 0xfff0;
  }
  else
  {
    *cl = (*cl & 0xf000) | 0x0fff;
  }
}


int copy_files(disk_sector_t *img, char *filenames[], int n_files)
{
  int i, k;
  FILE *f;
  int cl_start, cl_end;
  unsigned char *fat1, *fat2;
  root_entry_t *root;
  unsigned long n, size;

  fat1 = (unsigned char *)img[N_RESERVED];
  fat2 = (unsigned char *)img[N_RESERVED + SECTORS_PER_FAT];
  root = (root_entry_t *)img[N_RESERVED + N_FATS * SECTORS_PER_FAT];

  k = N_RESERVED +
      N_FATS * SECTORS_PER_FAT +
      N_ROOT_ENTRIES * ROOT_ENTRY_SIZE / SECTOR_SIZE;

  cl_end = 1;

  if (n_files > N_ROOT_ENTRIES)
  {
    n_files = N_ROOT_ENTRIES;
  }

  for (i = 0; i < n_files; i++)
  {
    cl_start = cl_end + 1;
    if ((f = fopen(filenames[i], "rb")) == NULL)
    {
      printf("Error opening file %s!", filenames[i]);
      return 1;
    }

    printf("  %s\n", filenames[i]);
    
    size = 0;
    while ((n = fread(img[k], 1, SECTOR_SIZE, f)) > 0)
    {
      size += n;
      cl_end++;
      k++;
    }
    fclose(f);

    root++;
    create_root_entry(root, filenames[i], cl_start, size);

    update_fat(fat1, cl_start, cl_end);
  }
  memcpy(fat2, fat1, SECTORS_PER_FAT * SECTOR_SIZE);

  return 0;
}


int write_image(disk_sector_t *img, char *imgname)
{
  FILE *f;

  if ((f = fopen(imgname, "rb")) != NULL)
  {
    printf("Image file %s already exists!\n", imgname);
    fclose(f);
    free(img);
    return 1;
  }

  f = fopen(imgname, "wb");
  if (fwrite(img, SECTOR_SIZE, SECTORS_PER_DISK, f) != SECTORS_PER_DISK)
  {
    printf("Unable to write image file %s\n!", imgname);
    fclose(f);
    free(img);
    return 1;
  }
  fclose(f);

  free(img);
  return 0;
}


int main(int argc, char *argv[])
{
  disk_sector_t *img;
  char *imgname;
  char *bsfname;
  char **filenames;
  int n_files;

  if (argc < 4)
  {
    printf("Usage: mkflpimg <image> <boot sector> <source files>\n");
    return 1;
  }

  imgname = argv[1];
  bsfname = argv[2];
  filenames = &argv[3];
  n_files = argc - 3;

  printf("Creating image ...\n");
  if ((img = new_image(bsfname)) == NULL)
  {
    return 1;
  }

  printf("Copying files ...\n");

  if (copy_files(img, filenames, n_files))
  {
    return 1;
  }

  printf("Writing image file ...\n");

  if (write_image(img, imgname))
  {
    return 1;
  }
  
  printf("Finished.\n");

  return 0;
}

