/**
 * makefsdata: Converts a directory structure for use with the lwIP httpd.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Jim Pettinato
 *         Simon Goldschmidt
 *
 * @todo:
 * - take TCP_MSS, LWIP_TCP_TIMESTAMPS and
 *   PAYLOAD_ALIGN_TYPE/PAYLOAD_ALIGNMENT as arguments
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "tinydir.h"

/** Makefsdata can generate *all* files deflate-compressed (where file size shrinks).
 * Since nearly all browsers support this, this is a good way to reduce ROM size.
 * To compress the files, "miniz.c" must be downloaded separately OR
 * MAKEFS_SUPPORT_DEFLATE_ZLIB must be set and the zlib library and headers
 * must be present on the system compiling this program.
 */
#ifndef MAKEFS_SUPPORT_DEFLATE
#define MAKEFS_SUPPORT_DEFLATE 0
#ifndef MAKEFS_SUPPORT_DEFLATE_ZLIB
#define MAKEFS_SUPPORT_DEFLATE_ZLIB 0
#endif /* MAKEFS_SUPPORT_DEFLATE_ZLIB */
#endif /* MAKEFS_SUPPORT_DEFLATE */

#define COPY_BUFSIZE (1024*1024) /* 1 MByte */

#if MAKEFS_SUPPORT_DEFLATE
#if MAKEFS_SUPPORT_DEFLATE_ZLIB
#include <zlib.h>
#else
#include "../miniz.c"
#endif /* MAKEFS_SUPPORT_DEFLATE */

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;

#define my_max(a,b) (((a) > (b)) ? (a) : (b))
#define my_min(a,b) (((a) < (b)) ? (a) : (b))

/* COMP_OUT_BUF_SIZE is the size of the output buffer used during compression.
   COMP_OUT_BUF_SIZE must be >= 1 and <= OUT_BUF_SIZE */
#define COMP_OUT_BUF_SIZE COPY_BUFSIZE

/* OUT_BUF_SIZE is the size of the output buffer used during decompression.
   OUT_BUF_SIZE must be a power of 2 >= TINFL_LZ_DICT_SIZE (because the low-level decompressor not only writes, but reads from the output buffer as it decompresses) */
#define OUT_BUF_SIZE COPY_BUFSIZE
static uint8 s_outbuf[OUT_BUF_SIZE];
static uint8 s_checkbuf[OUT_BUF_SIZE];

#ifndef MAKEFS_SUPPORT_DEFLATE_ZLIB
/* tdefl_compressor contains all the state needed by the low-level compressor so it's a pretty big struct (~300k).
   This example makes it a global vs. putting it on the stack, of course in real-world usage you'll probably malloc() or new it. */
tdefl_compressor g_deflator;
#endif /* MAKEFS_SUPPORT_DEFLATE_ZLIB */

static int deflate_level; /* default compression level, can be changed via command line */
#define USAGE_ARG_DEFLATE " [-defl<:compr_level>]"
#else /* MAKEFS_SUPPORT_DEFLATE */
#define USAGE_ARG_DEFLATE ""
#endif /* MAKEFS_SUPPORT_DEFLATE */

#ifdef WIN32

#define GETCWD(path, len)             GetCurrentDirectoryA(len, path)
#define GETCWD_SUCCEEDED(ret)         (ret != 0)
#define CHDIR(path)                   SetCurrentDirectoryA(path)
#define CHDIR_SUCCEEDED(ret)          (ret == TRUE)

#elif __linux__

#define GETCWD(path, len)             getcwd(path, len)
#define GETCWD_SUCCEEDED(ret)         (ret != NULL)
#define CHDIR(path)                   chdir(path)
#define CHDIR_SUCCEEDED(ret)          (ret == 0)

#else

#error makefsdata not supported on this platform

#endif

#define NEWLINE     "\r\n"
#define NEWLINE_LEN 2

/* Define this here since we don't include any external C files and ports might override it */
#define LWIP_PLATFORM_ASSERT(x) do {printf("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); fflush(NULL); abort();} while(0)

/* define this to get the header variables we use to build HTTP headers */
#define LWIP_HTTPD_DYNAMIC_HEADERS 1
#define LWIP_HTTPD_SSI             1
#include "lwip/init.h"
#include "../httpd_structs.h"
#include "lwip/apps/fs.h"

#include "../core/inet_chksum.c"
#include "../core/def.c"

/** (Your server name here) */
static const char *serverID = "Server: "HTTPD_SERVER_AGENT"\r\n";
static char serverIDBuffer[1024];

/* change this to suit your MEM_ALIGNMENT */
#define PAYLOAD_ALIGNMENT 4
/* set this to 0 to prevent aligning payload */
#define ALIGN_PAYLOAD 1
/* define this to a type that has the required alignment */
#define PAYLOAD_ALIGN_TYPE "unsigned int"
static int payload_alingment_dummy_counter = 0;

#define HEX_BYTES_PER_LINE 16

#define MAX_PATH_LEN 256

struct file_entry {
  struct file_entry *next;
  const char *filename_c;
};

int process_sub(FILE *data_file, FILE *struct_file);
int process_file(FILE *data_file, FILE *struct_file, const char *filename);
int file_write_http_header(FILE *data_file, const char *filename, int file_size, u16_t *http_hdr_len,
                           u16_t *http_hdr_chksum, u8_t provide_content_len, int is_compressed);
int file_put_ascii(FILE *file, const char *ascii_string, int len, int *i);
int s_put_ascii(char *buf, const char *ascii_string, int len, int *i);
void concat_files(const char *file1, const char *file2, const char *targetfile);
int check_path(char *path, size_t size);
static int checkSsiByFilelist(const char* filename_listfile);
static int ext_in_list(const char* filename, const char *ext_list);
static int file_to_exclude(const char* filename);
static int file_can_be_compressed(const char* filename);

/* 5 bytes per char + 3 bytes per line */
static char file_buffer_c[COPY_BUFSIZE * 5 + ((COPY_BUFSIZE / HEX_BYTES_PER_LINE) * 3)];

static char curSubdir[MAX_PATH_LEN-3];
static char lastFileVar[MAX_PATH_LEN];
static char hdr_buf[4096];

static unsigned char processSubs = 1;
static unsigned char includeHttpHeader = 1;
static unsigned char useHttp11 = 0;
static unsigned char supportSsi = 1;
static unsigned char precalcChksum = 0;
static unsigned char includeLastModified = 0;
#if MAKEFS_SUPPORT_DEFLATE
static unsigned char deflateNonSsiFiles = 0;
static size_t deflatedBytesReduced = 0;
static size_t overallDataBytes = 0;
#endif
static const char *exclude_list = NULL;
static const char *ncompress_list = NULL;

static struct file_entry *first_file = NULL;
static struct file_entry *last_file = NULL;

static char *ssi_file_buffer;
static char **ssi_file_lines;
static size_t ssi_file_num_lines;

static void print_usage(void)
{
  printf(" Usage: htmlgen [targetdir] [-s] [-e] [-11] [-nossi] [-ssi:<filename>] [-c] [-f:<filename>] [-m] [-svr:<name>] [-x:<ext_list>] [-xc:<ext_list>" USAGE_ARG_DEFLATE NEWLINE NEWLINE);
  printf("   targetdir: relative or absolute path to files to convert" NEWLINE);
  printf("   switch -s: toggle processing of subdirectories (default is on)" NEWLINE);
  printf("   switch -e: exclude HTTP header from file (header is created at runtime, default is off)" NEWLINE);
  printf("   switch -11: include HTTP 1.1 header (1.0 is default)" NEWLINE);
  printf("   switch -nossi: no support for SSI (cannot calculate Content-Length for SSI)" NEWLINE);
  printf("   switch -ssi: ssi filename (ssi support controlled by file list, not by extension)" NEWLINE);
  printf("   switch -c: precalculate checksums for all pages (default is off)" NEWLINE);
  printf("   switch -f: target filename (default is \"fsdata.c\")" NEWLINE);
  printf("   switch -m: include \"Last-Modified\" header based on file time" NEWLINE);
  printf("   switch -svr: server identifier sent in HTTP response header ('Server' field)" NEWLINE);
  printf("   switch -x: comma separated list of extensions of files to exclude (e.g., -x:json,txt)" NEWLINE);
  printf("   switch -xc: comma separated list of extensions of files to not compress (e.g., -xc:mp3,jpg)" NEWLINE);
#if MAKEFS_SUPPORT_DEFLATE
  printf("   switch -defl: deflate-compress all non-SSI files (with opt. compr.-level, default=10)" NEWLINE);
  printf("                 ATTENTION: browser has to support \"Content-Encoding: deflate\"!" NEWLINE);
#endif
  printf("   if targetdir not specified, htmlgen will attempt to" NEWLINE);
  printf("   process files in subdirectory 'fs'" NEWLINE);
}

int main(int argc, char *argv[])
{
  char path[MAX_PATH_LEN];
  char appPath[MAX_PATH_LEN];
  FILE *data_file;
  FILE *struct_file;
  int filesProcessed;
  int i;
  char targetfile[MAX_PATH_LEN];
  strcpy(targetfile, "fsdata.c");

  memset(path, 0, sizeof(path));
  memset(appPath, 0, sizeof(appPath));

  printf(NEWLINE " makefsdata v" LWIP_VERSION_STRING " - HTML to C source converter" NEWLINE);
  printf("     by Jim Pettinato               - circa 2003 " NEWLINE);
  printf("     extended by Simon Goldschmidt  - 2009 " NEWLINE NEWLINE);

  LWIP_ASSERT("sizeof(hdr_buf) must fit into an u16_t", sizeof(hdr_buf) <= 0xffff);

  strcpy(path, "fs");
  for (i = 1; i < argc; i++) {
    if (argv[i] == NULL) {
      continue;
    }
    if (argv[i][0] == '-') {
      if (strstr(argv[i], "-svr:") == argv[i]) {
        snprintf(serverIDBuffer, sizeof(serverIDBuffer), "Server: %s\r\n", &argv[i][5]);
        serverID = serverIDBuffer;
        printf("Using Server-ID: \"%s\"\n", serverID);
      } else if (!strcmp(argv[i], "-s")) {
        processSubs = 0;
      } else if (!strcmp(argv[i], "-e")) {
        includeHttpHeader = 0;
      } else if (!strcmp(argv[i], "-11")) {
        useHttp11 = 1;
      } else if (!strcmp(argv[i], "-nossi")) {
        supportSsi = 0;
      } else if (strstr(argv[i], "-ssi:") == argv[i]) {
        const char* ssi_list_filename = &argv[i][5];
        if (checkSsiByFilelist(ssi_list_filename)) {
          printf("Reading list of SSI files from \"%s\"\n", ssi_list_filename);
        } else {
          printf("Failed to load list of SSI files from \"%s\"\n", ssi_list_filename);
        }
      } else if (!strcmp(argv[i], "-c")) {
        precalcChksum = 1;
      } else if (strstr(argv[i], "-f:") == argv[i]) {
        strncpy(targetfile, &argv[i][3], sizeof(targetfile) - 1);
        targetfile[sizeof(targetfile) - 1] = 0;
        printf("Writing to file \"%s\"\n", targetfile);
      } else if (!strcmp(argv[i], "-m")) {
        includeLastModified = 1;
      } else if (strstr(argv[i], "-defl") == argv[i]) {
#if MAKEFS_SUPPORT_DEFLATE
        const char *colon = &argv[i][5];
        if (*colon == ':') {
          int defl_level = atoi(&colon[1]);
          if ((colon[1] != 0) && (defl_level >= 0) && (defl_level <= 10)) {
            deflate_level = defl_level;
          } else {
            printf("ERROR: deflate level must be [0..10]" NEWLINE);
            exit(0);
          }
        } else {
          /* default to highest compression */
          deflate_level = 10;
        }
        deflateNonSsiFiles = 1;
        printf("Deflating all non-SSI files with level %d (but only if size is reduced)" NEWLINE, deflate_level);
#else
        printf("WARNING: Deflate support is disabled\n");
#endif
      } else if (strstr(argv[i], "-x:") == argv[i]) {
        exclude_list = &argv[i][3];
        printf("Excluding files with extensions %s" NEWLINE, exclude_list);
      } else if (strstr(argv[i], "-xc:") == argv[i]) {
        ncompress_list = &argv[i][4];
        printf("Skipping compression for files with extensions %s" NEWLINE, ncompress_list);
      } else if ((strstr(argv[i], "-?")) || (strstr(argv[i], "-h"))) {
        print_usage();
        exit(0);
      }
    } else if ((argv[i][0] == '/') && (argv[i][1] == '?') && (argv[i][2] == 0)) {
      print_usage();
      exit(0);
    } else {
      strncpy(path, argv[i], sizeof(path) - 1);
      path[sizeof(path) - 1] = 0;
    }
  }

  if (!check_path(path, sizeof(path))) {
    printf("Invalid path: \"%s\"." NEWLINE, path);
    exit(-1);
  }

  if(!GETCWD_SUCCEEDED(GETCWD(appPath, MAX_PATH_LEN))) {
    printf("Unable to get current dir." NEWLINE);
    exit(-1);
  }
  /* if command line param or subdir named 'fs' not found spout usage verbiage */
  if (!CHDIR_SUCCEEDED(CHDIR(path))) {
    /* if no subdir named 'fs' (or the one which was given) exists, spout usage verbiage */
    printf(" Failed to open directory \"%s\"." NEWLINE NEWLINE, path);
    print_usage();
    exit(-1);
  }
  if(!CHDIR_SUCCEEDED(CHDIR(appPath))) {
    printf("Invalid path: \"%s\"." NEWLINE, appPath);
    exit(-1);
  }

  printf("HTTP %sheader will %s statically included." NEWLINE,
         (includeHttpHeader ? (useHttp11 ? "1.1 " : "1.0 ") : ""),
         (includeHttpHeader ? "be" : "not be"));

  curSubdir[0] = '\0'; /* start off in web page's root directory - relative paths */
  printf("  Processing all files in directory %s", path);
  if (processSubs) {
    printf(" and subdirectories..." NEWLINE NEWLINE);
  } else {
    printf("..." NEWLINE NEWLINE);
  }

  data_file = fopen("fsdata.tmp", "wb");
  if (data_file == NULL) {
    printf("Failed to create file \"fsdata.tmp\"\n");
    exit(-1);
  }
  struct_file = fopen("fshdr.tmp", "wb");
  if (struct_file == NULL) {
    printf("Failed to create file \"fshdr.tmp\"\n");
    fclose(data_file);
    exit(-1);
  }

  if(!CHDIR_SUCCEEDED(CHDIR(path))) {
    printf("Invalid path: \"%s\"." NEWLINE, path);
    exit(-1);
  }

  fprintf(data_file, "#include \"lwip/apps/fs.h\"" NEWLINE);
  fprintf(data_file, "#include \"lwip/def.h\"" NEWLINE NEWLINE NEWLINE);

  fprintf(data_file, "#define file_NULL (struct fsdata_file *) NULL" NEWLINE NEWLINE NEWLINE);
  /* define FS_FILE_FLAGS_HEADER_INCLUDED to 1 if not defined (compatibility with older httpd/fs) */
  fprintf(data_file, "#ifndef FS_FILE_FLAGS_HEADER_INCLUDED" NEWLINE "#define FS_FILE_FLAGS_HEADER_INCLUDED 1" NEWLINE "#endif" NEWLINE);
  /* define FS_FILE_FLAGS_HEADER_PERSISTENT to 0 if not defined (compatibility with older httpd/fs: wasn't supported back then) */
  fprintf(data_file, "#ifndef FS_FILE_FLAGS_HEADER_PERSISTENT" NEWLINE "#define FS_FILE_FLAGS_HEADER_PERSISTENT 0" NEWLINE "#endif" NEWLINE);

  /* define alignment defines */
#if ALIGN_PAYLOAD
  fprintf(data_file, "/* FSDATA_FILE_ALIGNMENT: 0=off, 1=by variable, 2=by include */" NEWLINE "#ifndef FSDATA_FILE_ALIGNMENT" NEWLINE "#define FSDATA_FILE_ALIGNMENT 0" NEWLINE "#endif" NEWLINE);
#endif
  fprintf(data_file, "#ifndef FSDATA_ALIGN_PRE"  NEWLINE "#define FSDATA_ALIGN_PRE"  NEWLINE "#endif" NEWLINE);
  fprintf(data_file, "#ifndef FSDATA_ALIGN_POST" NEWLINE "#define FSDATA_ALIGN_POST" NEWLINE "#endif" NEWLINE);
#if ALIGN_PAYLOAD
  fprintf(data_file, "#if FSDATA_FILE_ALIGNMENT==2" NEWLINE "#include \"fsdata_alignment.h\"" NEWLINE "#endif" NEWLINE);
#endif

  sprintf(lastFileVar, "NULL");

  filesProcessed = process_sub(data_file, struct_file);

  /* data_file now contains all of the raw data.. now append linked list of
   * file header structs to allow embedded app to search for a file name */
  fprintf(data_file, NEWLINE NEWLINE);
  fprintf(struct_file, "#define FS_ROOT file_%s" NEWLINE, lastFileVar);
  fprintf(struct_file, "#define FS_NUMFILES %d" NEWLINE NEWLINE, filesProcessed);

  fclose(data_file);
  fclose(struct_file);

  if(!CHDIR_SUCCEEDED(CHDIR(appPath))) {
    printf("Invalid path: \"%s\"." NEWLINE, appPath);
    exit(-1);
  }

  /* append struct_file to data_file */
  printf(NEWLINE "Creating target file..." NEWLINE NEWLINE);
  concat_files("fsdata.tmp", "fshdr.tmp", targetfile);

  /* if succeeded, delete the temporary files */
  if (remove("fsdata.tmp") != 0) {
    printf("Warning: failed to delete fsdata.tmp\n");
  }
  if (remove("fshdr.tmp") != 0) {
    printf("Warning: failed to delete fshdr.tmp\n");
  }

  printf(NEWLINE "Processed %d files - done." NEWLINE, filesProcessed);
#if MAKEFS_SUPPORT_DEFLATE
  if (deflateNonSsiFiles) {
    printf("(Deflated total byte reduction: %d bytes -> %d bytes (%.02f%%)" NEWLINE,
           (int)overallDataBytes, (int)deflatedBytesReduced, (float)((deflatedBytesReduced * 100.0) / overallDataBytes));
  }
#endif
  printf(NEWLINE);

  while (first_file != NULL) {
    struct file_entry *fe = first_file;
    first_file = fe->next;
    free(fe);
  }

  if (ssi_file_buffer) {
    free(ssi_file_buffer);
  }
  if (ssi_file_lines) {
    free(ssi_file_lines);
  }

  return 0;
}

int check_path(char *path, size_t size)
{
  size_t slen;
  if (path[0] == 0) {
    /* empty */
    return 0;
  }
  slen = strlen(path);
  if (slen >= size) {
    /* not NULL-terminated */
    return 0;
  }
  while ((slen > 0) && ((path[slen] == '\\') || (path[slen] == '/'))) {
    /* path should not end with trailing backslash */
    path[slen] = 0;
    slen--;
  }
  if (slen == 0) {
    return 0;
  }
  return 1;
}

static void copy_file(const char *filename_in, FILE *fout)
{
  FILE *fin;
  size_t len;
  void *buf;
  fin = fopen(filename_in, "rb");
  if (fin == NULL) {
    printf("Failed to open file \"%s\"\n", filename_in);
    exit(-1);
  }
  buf = malloc(COPY_BUFSIZE);
  while ((len = fread(buf, 1, COPY_BUFSIZE, fin)) > 0) {
    fwrite(buf, 1, len, fout);
  }
  free(buf);
  fclose(fin);
}

void concat_files(const char *file1, const char *file2, const char *targetfile)
{
  FILE *fout;
  fout = fopen(targetfile, "wb");
  if (fout == NULL) {
    printf("Failed to open file \"%s\"\n", targetfile);
    exit(-1);
  }
  copy_file(file1, fout);
  copy_file(file2, fout);
  fclose(fout);
}

int process_sub(FILE *data_file, FILE *struct_file)
{
  tinydir_dir dir;
  int filesProcessed = 0;

  if (processSubs) {
    /* process subs recursively */
    size_t sublen = strlen(curSubdir);
    size_t freelen = sizeof(curSubdir) - sublen - 1;
    int ret;
    LWIP_ASSERT("sublen < sizeof(curSubdir)", sublen < sizeof(curSubdir));

    ret = tinydir_open_sorted(&dir, TINYDIR_STRING("."));

    if (ret == 0) {
      unsigned int i;
      for (i = 0; i < dir.n_files; i++) {
        tinydir_file file;

        ret = tinydir_readfile_n(&dir, &file, i);

        if (ret == 0) {
#if (defined _MSC_VER || defined __MINGW32__) && (defined _UNICODE)
          size_t num_char_converted;
          char currName[256];
          wcstombs_s(&num_char_converted, currName, sizeof(currName), file.name, sizeof(currName));
#else
          const char *currName = file.name;
#endif

          if (currName[0] == '.') {
            continue;
          }
          if (!file.is_dir) {
            continue;
          }
          if (freelen > 0) {
            if(!CHDIR_SUCCEEDED(CHDIR(currName))) {
              printf("Invalid path: \"%s\"." NEWLINE, currName);
              exit(-1);
            }
            strncat(curSubdir, "/", freelen);
            strncat(curSubdir, currName, freelen - 1);
            curSubdir[sizeof(curSubdir) - 1] = 0;
            printf("processing subdirectory %s/..." NEWLINE, curSubdir);
            filesProcessed += process_sub(data_file, struct_file);
            if(!CHDIR_SUCCEEDED(CHDIR(".."))) {
              printf("Unable to get back to parent dir of: \"%s\"." NEWLINE, currName);
              exit(-1);
            }
            curSubdir[sublen] = 0;
          } else {
            printf("WARNING: cannot process sub due to path length restrictions: \"%s/%s\"\n", curSubdir, currName);
          }
        }
      }
    }

    ret = tinydir_open_sorted(&dir, TINYDIR_STRING("."));
    if (ret == 0) {
      unsigned int i;
      for (i = 0; i < dir.n_files; i++) {
        tinydir_file file;

        ret = tinydir_readfile_n(&dir, &file, i);

        if (ret == 0) {
          if (!file.is_dir) {
#if (defined _MSC_VER || defined __MINGW32__) && (defined _UNICODE)
            size_t num_char_converted;
            char curName[256];
            wcstombs_s(&num_char_converted, curName, sizeof(curName), file.name, sizeof(curName));
#else
            const char *curName = file.name;
#endif

            if (strcmp(curName, "fsdata.tmp") == 0) {
              continue;
            }
            if (strcmp(curName, "fshdr.tmp") == 0) {
              continue;
            }
            if (file_to_exclude(curName)) {
              printf("skipping %s/%s by exclude list (-x option)..." NEWLINE, curSubdir, curName);
              continue;
            }

            printf("processing %s/%s..." NEWLINE, curSubdir, curName);

            if (process_file(data_file, struct_file, curName) < 0) {
              printf(NEWLINE "Error... aborting" NEWLINE);
              return -1;
            }
            filesProcessed++;
          }
        }
      }
    }
  }

  return filesProcessed;
}

static u8_t *get_file_data(const char *filename, int *file_size, int can_be_compressed, int *is_compressed)
{
  FILE *inFile;
  size_t fsize = 0;
  u8_t *buf;
  size_t r;
  int rs;
  LWIP_UNUSED_ARG(r); /* for LWIP_NOASSERT */
  inFile = fopen(filename, "rb");
  if (inFile == NULL) {
    printf("Failed to open file \"%s\"\n", filename);
    exit(-1);
  }
  fseek(inFile, 0, SEEK_END);
  rs = ftell(inFile);
  if (rs < 0) {
    printf("ftell failed with %d\n", errno);
    exit(-1);
  }
  fsize = (size_t)rs;
  fseek(inFile, 0, SEEK_SET);
  buf = (u8_t *)malloc(fsize);
  LWIP_ASSERT("buf != NULL", buf != NULL);
  r = fread(buf, 1, fsize, inFile);
  LWIP_ASSERT("r == fsize", r == fsize);
  *file_size = fsize;
  *is_compressed = 0;
#if MAKEFS_SUPPORT_DEFLATE
  overallDataBytes += fsize;
  if (deflateNonSsiFiles) {
    if (can_be_compressed) {
      if (fsize < OUT_BUF_SIZE) {
        u8_t *ret_buf;
#ifndef MAKEFS_SUPPORT_DEFLATE_ZLIB
        tdefl_status status;
#else /* MAKEFS_SUPPORT_DEFLATE_ZLIB */
        int status;
#endif /* MAKEFS_SUPPORT_DEFLATE_ZLIB */
        size_t in_bytes = fsize;
        size_t out_bytes = OUT_BUF_SIZE;
        const void *next_in = buf;
        void *next_out = s_outbuf;
        memset(s_outbuf, 0, sizeof(s_outbuf));
#ifndef MAKEFS_SUPPORT_DEFLATE_ZLIB
        /* create tdefl() compatible flags (we have to compose the low-level flags ourselves, or use tdefl_create_comp_flags_from_zip_params() but that means MINIZ_NO_ZLIB_APIS can't be defined). */
        mz_uint comp_flags = s_tdefl_num_probes[MZ_MIN(10, deflate_level)] | ((deflate_level <= 3) ? TDEFL_GREEDY_PARSING_FLAG : 0);
        if (!deflate_level) {
          comp_flags |= TDEFL_FORCE_ALL_RAW_BLOCKS;
        }
        status = tdefl_init(&g_deflator, NULL, NULL, comp_flags);
        if (status != TDEFL_STATUS_OKAY) {
          printf("tdefl_init() failed!\n");
          exit(-1);
        }
        status = tdefl_compress(&g_deflator, next_in, &in_bytes, next_out, &out_bytes, TDEFL_FINISH);
        if (status != TDEFL_STATUS_DONE) {
          printf("deflate failed: %d\n", status);
          exit(-1);
        }
#else /* MAKEFS_SUPPORT_DEFLATE_ZLIB */
        status = compress2(next_out, &out_bytes, next_in, in_bytes, deflate_level);
        if (status != Z_OK) {
          printf("deflate failed: %d\n", status);
          exit(-1);
        }
#endif /*  MAKEFS_SUPPORT_DEFLATE_ZLIB */
        LWIP_ASSERT("out_bytes <= COPY_BUFSIZE", out_bytes <= OUT_BUF_SIZE);
        if (out_bytes < fsize) {
          ret_buf = (u8_t *)malloc(out_bytes);
          LWIP_ASSERT("ret_buf != NULL", ret_buf != NULL);
          memcpy(ret_buf, s_outbuf, out_bytes);
          {
            /* sanity-check compression be inflating and comparing to the original */
            size_t dec_in_bytes = out_bytes;
            size_t dec_out_bytes = OUT_BUF_SIZE;
            next_out = s_checkbuf;
            memset(s_checkbuf, 0, sizeof(s_checkbuf));
#ifndef MAKEFS_SUPPORT_DEFLATE_ZLIB
            tinfl_status dec_status;
            tinfl_decompressor inflator;

            tinfl_init(&inflator);
            dec_status = tinfl_decompress(&inflator, (const mz_uint8 *)ret_buf, &dec_in_bytes, s_checkbuf, (mz_uint8 *)next_out, &dec_out_bytes, 0);
            LWIP_ASSERT("tinfl_decompress failed", dec_status == TINFL_STATUS_DONE);
#else /* MAKEFS_SUPPORT_DEFLATE_ZLIB */
            int dec_status;
            dec_status = uncompress2 (s_checkbuf, &dec_out_bytes, ret_buf, &dec_in_bytes);
            LWIP_ASSERT("tinfl_decompress failed", dec_status == Z_OK);
#endif /* MAKEFS_SUPPORT_DEFLATE_ZLIB */
            LWIP_ASSERT("tinfl_decompress size mismatch", fsize == dec_out_bytes);
            LWIP_ASSERT("decompressed memcmp failed", !memcmp(s_checkbuf, buf, fsize));
          }
          /* free original buffer, use compressed data + size */
          free(buf);
          buf = ret_buf;
          *file_size = out_bytes;
          printf(" - deflate: %d bytes -> %d bytes (%.02f%%)" NEWLINE, (int)fsize, (int)out_bytes, (float)((out_bytes * 100.0) / fsize));
          deflatedBytesReduced += (size_t)(fsize - out_bytes);
          *is_compressed = 1;
        } else {
          printf(" - uncompressed: (would be %d bytes larger using deflate)" NEWLINE, (int)(out_bytes - fsize));
        }
      } else {
        printf(" - uncompressed: (file is larger than deflate buffer)" NEWLINE);
      }
    } else {
      printf(" - cannot be compressed" NEWLINE);
    }
  }
#else
  LWIP_UNUSED_ARG(can_be_compressed);
#endif
  fclose(inFile);
  return buf;
}

static void process_file_data(FILE *data_file, u8_t *file_data, size_t file_size)
{
  size_t written, i, src_off = 0;
  size_t off = 0;
  LWIP_UNUSED_ARG(written); /* for LWIP_NOASSERT */
  for (i = 0; i < file_size; i++) {
    LWIP_ASSERT("file_buffer_c overflow", off < sizeof(file_buffer_c) - 5);
    sprintf(&file_buffer_c[off], "0x%02x,", file_data[i]);
    off += 5;
    if ((++src_off % HEX_BYTES_PER_LINE) == 0) {
      LWIP_ASSERT("file_buffer_c overflow", off < sizeof(file_buffer_c) - NEWLINE_LEN);
      memcpy(&file_buffer_c[off], NEWLINE, NEWLINE_LEN);
      off += NEWLINE_LEN;
    }
    if (off + 20 >= sizeof(file_buffer_c)) {
      written = fwrite(file_buffer_c, 1, off, data_file);
      LWIP_ASSERT("written == off", written == off);
      off = 0;
    }
  }
  written = fwrite(file_buffer_c, 1, off, data_file);
  LWIP_ASSERT("written == off", written == off);
}

static int write_checksums(FILE *struct_file, const char *varname,
                           u16_t hdr_len, u16_t hdr_chksum, const u8_t *file_data, size_t file_size)
{
  int chunk_size = TCP_MSS;
  int offset, src_offset;
  size_t len;
  int i = 0;
#if LWIP_TCP_TIMESTAMPS
  /* when timestamps are used, usable space is 12 bytes less per segment */
  chunk_size -= 12;
#endif

  fprintf(struct_file, "#if HTTPD_PRECALCULATED_CHECKSUM" NEWLINE);
  fprintf(struct_file, "const struct fsdata_chksum chksums_%s[] = {" NEWLINE, varname);

  if (hdr_len > 0) {
    /* add checksum for HTTP header */
    fprintf(struct_file, "{%d, 0x%04x, %d}," NEWLINE, 0, hdr_chksum, hdr_len);
    i++;
  }
  src_offset = 0;
  for (offset = hdr_len; ; offset += len) {
    unsigned short chksum;
    const void *data = (const void *)&file_data[src_offset];
    len = LWIP_MIN(chunk_size, (int)file_size - src_offset);
    if (len == 0) {
      break;
    }
    chksum = ~inet_chksum(data, (u16_t)len);
    /* add checksum for data */
    fprintf(struct_file, "{%d, 0x%04x, %"SZT_F"}," NEWLINE, offset, chksum, len);
    i++;
  }
  fprintf(struct_file, "};" NEWLINE);
  fprintf(struct_file, "#endif /* HTTPD_PRECALCULATED_CHECKSUM */" NEWLINE);
  return i;
}

static int is_valid_char_for_c_var(char x)
{
  if (((x >= 'A') && (x <= 'Z')) ||
      ((x >= 'a') && (x <= 'z')) ||
      ((x >= '0') && (x <= '9')) ||
      (x == '_')) {
    return 1;
  }
  return 0;
}

static void fix_filename_for_c(char *qualifiedName, size_t max_len)
{
  struct file_entry *f;
  size_t len = strlen(qualifiedName);
  char *new_name = (char *)malloc(len + 2);
  int filename_ok;
  int cnt = 0;
  size_t i;
  if (len + 3 == max_len) {
    printf("File name too long: \"%s\"\n", qualifiedName);
    exit(-1);
  }
  strcpy(new_name, qualifiedName);
  for (i = 0; i < len; i++) {
    if (!is_valid_char_for_c_var(new_name[i])) {
      new_name[i] = '_';
    }
  }
  do {
    filename_ok = 1;
    for (f = first_file; f != NULL; f = f->next) {
      if (!strcmp(f->filename_c, new_name)) {
        filename_ok = 0;
        cnt++;
        /* try next unique file name */
        sprintf(&new_name[len], "%d", cnt);
        break;
      }
    }
  } while (!filename_ok && (cnt < 999));
  if (!filename_ok) {
    printf("Failed to get unique file name: \"%s\"\n", qualifiedName);
    exit(-1);
  }
  strcpy(qualifiedName, new_name);
  free(new_name);
}

static void register_filename(const char *qualifiedName)
{
  struct file_entry *fe = (struct file_entry *)malloc(sizeof(struct file_entry));
  fe->filename_c = strdup(qualifiedName);
  fe->next = NULL;
  if (first_file == NULL) {
    first_file = last_file = fe;
  } else {
    last_file->next = fe;
    last_file = fe;
  }
}

static int checkSsiByFilelist(const char* filename_listfile)
{
  FILE *f = fopen(filename_listfile, "r");
  if (f != NULL) {
    char *buf;
    long rs;
    size_t fsize, readcount;
    size_t i, l, num_lines;
    char **lines;
    int state;

    fseek(f, 0, SEEK_END);
    rs = ftell(f);
    if (rs < 0) {
      printf("ftell failed with %d\n", errno);
      fclose(f);
      return 0;
    }
    fsize = (size_t)rs;
    fseek(f, 0, SEEK_SET);
    buf = (char*)malloc(fsize);
    if (!buf) {
      printf("failed to allocate ssi file buffer\n");
      fclose(f);
      return 0;
    }
    memset(buf, 0, fsize);
    readcount = fread(buf, 1, fsize, f);
    fclose(f);
    if ((readcount > fsize) || !readcount) {
      printf("failed to read data from ssi file\n");
      free(buf);
      return 0;
    }

    /* first pass: get the number of lines (and convert newlines to '0') */
    num_lines = 1;
    for (i = 0; i < readcount; i++) {
      if (buf[i] == '\n') {
        num_lines++;
        buf[i] = 0;
      } else if (buf[i] == '\r') {
        buf[i] = 0;
      }
    }
    /* allocate the line pointer array */
    lines = (char**)malloc(sizeof(char*) * num_lines);
    if (!lines) {
      printf("failed to allocate ssi line buffer\n");
      free(buf);
      return 0;
    }
    memset(lines, 0, sizeof(char*) * num_lines);
    l = 0;
    state = 0;
    for (i = 0; i < readcount; i++) {
      if (state) {
        /* waiting for null */
        if (buf[i] == 0) {
          state = 0;
        }
      } else {
        /* waiting for beginning of new string */
        if (buf[i] != 0) {
          LWIP_ASSERT("lines array overflow", l < num_lines);
          lines[l] = &buf[i];
          state = 1;
          l++;
        }
      }
    }
    LWIP_ASSERT("lines array overflow", l < num_lines);

    ssi_file_buffer = buf;
    ssi_file_lines = lines;
    ssi_file_num_lines = l;
  }
  return 0;
}

static int is_ssi_file(const char *filename)
{
  if (supportSsi) {
    if (ssi_file_buffer) {
      /* compare by list */
      size_t i;
      int ret = 0;
      /* build up the relative path to this file */
      size_t sublen = strlen(curSubdir);
      size_t freelen = sizeof(curSubdir) - sublen - 1;
      strncat(curSubdir, "/", freelen);
      strncat(curSubdir, filename, freelen - 1);
      curSubdir[sizeof(curSubdir) - 1] = 0;
      for (i = 0; i < ssi_file_num_lines; i++) {
        const char *listed_file = ssi_file_lines[i];
        /* compare without the leading '/' */
        if (!strcmp(&curSubdir[1], listed_file)) {
          ret = 1;
        }
      }
      curSubdir[sublen] = 0;
      return ret;
#if LWIP_HTTPD_SSI_BY_FILE_EXTENSION
    } else {
      /* check file extension */
      size_t loop;
      for (loop = 0; loop < NUM_SHTML_EXTENSIONS; loop++) {
        if (strstr(filename, g_pcSSIExtensions[loop])) {
          return 1;
        }
      }
#endif /* LWIP_HTTPD_SSI_BY_FILE_EXTENSION */
    }
  }
  return 0;
}

static int ext_in_list(const char* filename, const char *ext_list)
{
  int found = 0;
  const char *ext = ext_list;
  if (ext_list == NULL) {
    return 0;
  }
  while(*ext != '\0') {
    const char *comma = strchr(ext, ',');
    size_t ext_size;
    size_t filename_size = strlen(filename);
    if (comma == NULL) {
      comma = strchr(ext, '\0');
    }
    ext_size = comma - ext;
    if ((filename[filename_size - ext_size - 1] == '.') &&
      !strncmp(&filename[filename_size - ext_size], ext, ext_size)) {
        found = 1;
        break;
    }
    ext = comma + 1;
  }

  return found;
}

static int file_to_exclude(const char *filename)
{
    return (exclude_list != NULL) && ext_in_list(filename, exclude_list);
}

static int file_can_be_compressed(const char *filename)
{
    return (ncompress_list == NULL) || !ext_in_list(filename, ncompress_list);
}

int process_file(FILE *data_file, FILE *struct_file, const char *filename)
{
  char varname[MAX_PATH_LEN];
  int i = 0;
  char qualifiedName[MAX_PATH_LEN];
  int file_size;
  u16_t http_hdr_chksum = 0;
  u16_t http_hdr_len = 0;
  int chksum_count = 0;
  u8_t flags = 0;
  u8_t has_content_len;
  u8_t *file_data;
  int is_ssi;
  int can_be_compressed;
  int is_compressed = 0;
  int flags_printed;

  /* create qualified name (@todo: prepend slash or not?) */
  snprintf(qualifiedName, sizeof(qualifiedName), "%s/%s", curSubdir, filename);
  /* create C variable name */
  strncpy(varname, qualifiedName, sizeof(varname));
  /* convert slashes & dots to underscores */
  fix_filename_for_c(varname, MAX_PATH_LEN);
  register_filename(varname);
#if ALIGN_PAYLOAD
  /* to force even alignment of array, type 1 */
  fprintf(data_file, "#if FSDATA_FILE_ALIGNMENT==1" NEWLINE);
  fprintf(data_file, "static const " PAYLOAD_ALIGN_TYPE " dummy_align_%s = %d;" NEWLINE, varname, payload_alingment_dummy_counter++);
  fprintf(data_file, "#endif" NEWLINE);
#endif /* ALIGN_PAYLOAD */
  fprintf(data_file, "static const unsigned char FSDATA_ALIGN_PRE data_%s[] FSDATA_ALIGN_POST = {" NEWLINE, varname);
  /* encode source file name (used by file system, not returned to browser) */
  fprintf(data_file, "/* %s (%"SZT_F" chars) */" NEWLINE, qualifiedName, strlen(qualifiedName) + 1);
  file_put_ascii(data_file, qualifiedName, strlen(qualifiedName) + 1, &i);
#if ALIGN_PAYLOAD
  /* pad to even number of bytes to assure payload is on aligned boundary */
  while (i % PAYLOAD_ALIGNMENT != 0) {
    fprintf(data_file, "0x%02x,", 0);
    i++;
  }
#endif /* ALIGN_PAYLOAD */
  fprintf(data_file, NEWLINE);

  is_ssi = is_ssi_file(filename);
  if (is_ssi) {
    flags |= FS_FILE_FLAGS_SSI;
  }
  has_content_len = !is_ssi;
  can_be_compressed = includeHttpHeader && !is_ssi && file_can_be_compressed(filename);
  file_data = get_file_data(filename, &file_size, can_be_compressed, &is_compressed);
  if (includeHttpHeader) {
    file_write_http_header(data_file, filename, file_size, &http_hdr_len, &http_hdr_chksum, has_content_len, is_compressed);
    flags |= FS_FILE_FLAGS_HEADER_INCLUDED;
    if (has_content_len) {
      flags |= FS_FILE_FLAGS_HEADER_PERSISTENT;
      if (useHttp11) {
        flags |= FS_FILE_FLAGS_HEADER_HTTPVER_1_1;
      }
    }
  }
  if (precalcChksum) {
    chksum_count = write_checksums(struct_file, varname, http_hdr_len, http_hdr_chksum, file_data, file_size);
  }

  /* build declaration of struct fsdata_file in temp file */
  fprintf(struct_file, "const struct fsdata_file file_%s[] = { {" NEWLINE, varname);
  fprintf(struct_file, "file_%s," NEWLINE, lastFileVar);
  fprintf(struct_file, "data_%s," NEWLINE, varname);
  fprintf(struct_file, "data_%s + %d," NEWLINE, varname, i);
  fprintf(struct_file, "sizeof(data_%s) - %d," NEWLINE, varname, i);

  flags_printed = 0;
  if (flags & FS_FILE_FLAGS_HEADER_INCLUDED) {
    fputs("FS_FILE_FLAGS_HEADER_INCLUDED", struct_file);
    flags_printed = 1;
  }
  if (flags & FS_FILE_FLAGS_HEADER_PERSISTENT) {
    if (flags_printed) {
      fputs(" | ", struct_file);
    }
    fputs("FS_FILE_FLAGS_HEADER_PERSISTENT", struct_file);
    flags_printed = 1;
  }
  if (flags & FS_FILE_FLAGS_HEADER_HTTPVER_1_1) {
    if (flags_printed) {
      fputs(" | ", struct_file);
    }
    fputs("FS_FILE_FLAGS_HEADER_HTTPVER_1_1", struct_file);
    flags_printed = 1;
  }
  if (flags & FS_FILE_FLAGS_SSI) {
    if (flags_printed) {
      fputs(" | ", struct_file);
    }
    fputs("FS_FILE_FLAGS_SSI", struct_file);
    flags_printed = 1;
  }
  if (!flags_printed) {
    fputs("0", struct_file);
  }
  fputs("," NEWLINE, struct_file);
  if (precalcChksum) {
    fprintf(struct_file, "#if HTTPD_PRECALCULATED_CHECKSUM" NEWLINE);
    fprintf(struct_file, "%d, chksums_%s," NEWLINE, chksum_count, varname);
    fprintf(struct_file, "#endif /* HTTPD_PRECALCULATED_CHECKSUM */" NEWLINE);
  }
  fprintf(struct_file, "}};" NEWLINE NEWLINE);
  strcpy(lastFileVar, varname);

  /* write actual file contents */
  i = 0;
  fprintf(data_file, NEWLINE "/* raw file data (%d bytes) */" NEWLINE, file_size);
  process_file_data(data_file, file_data, file_size);
  fprintf(data_file, "};" NEWLINE NEWLINE);
  free(file_data);
  return 0;
}

int file_write_http_header(FILE *data_file, const char *filename, int file_size, u16_t *http_hdr_len,
                           u16_t *http_hdr_chksum, u8_t provide_content_len, int is_compressed)
{
  int i = 0;
  int response_type = HTTP_HDR_OK;
  const char *file_type;
  const char *cur_string;
  size_t cur_len;
  int written = 0;
  size_t hdr_len = 0;
  u16_t acc;
  const char *file_ext;
  size_t j;
  u8_t provide_last_modified = includeLastModified;

  memset(hdr_buf, 0, sizeof(hdr_buf));

  if (useHttp11) {
    response_type = HTTP_HDR_OK_11;
  }

  fprintf(data_file, NEWLINE "/* HTTP header */");
  if (strstr(filename, "404.") == filename) {
    response_type = HTTP_HDR_NOT_FOUND;
    if (useHttp11) {
      response_type = HTTP_HDR_NOT_FOUND_11;
    }
  } else if (strstr(filename, "400.") == filename) {
    response_type = HTTP_HDR_BAD_REQUEST;
    if (useHttp11) {
      response_type = HTTP_HDR_BAD_REQUEST_11;
    }
  } else if (strstr(filename, "501.") == filename) {
    response_type = HTTP_HDR_NOT_IMPL;
    if (useHttp11) {
      response_type = HTTP_HDR_NOT_IMPL_11;
    }
  }
  cur_string = g_psHTTPHeaderStrings[response_type];
  cur_len = strlen(cur_string);
  fprintf(data_file, NEWLINE "/* \"%s\" (%"SZT_F" bytes) */" NEWLINE, cur_string, cur_len);
  written += file_put_ascii(data_file, cur_string, cur_len, &i);
  i = 0;
  if (precalcChksum) {
    memcpy(&hdr_buf[hdr_len], cur_string, cur_len);
    hdr_len += cur_len;
  }

  cur_string = serverID;
  cur_len = strlen(cur_string);
  fprintf(data_file, NEWLINE "/* \"%s\" (%"SZT_F" bytes) */" NEWLINE, cur_string, cur_len);
  written += file_put_ascii(data_file, cur_string, cur_len, &i);
  i = 0;
  if (precalcChksum) {
    memcpy(&hdr_buf[hdr_len], cur_string, cur_len);
    hdr_len += cur_len;
  }

  file_ext = filename;
  if (file_ext != NULL) {
    while (strstr(file_ext, ".") != NULL) {
      file_ext = strstr(file_ext, ".");
      file_ext++;
    }
  }
  if ((file_ext == NULL) || (*file_ext == 0)) {
    printf("failed to get extension for file \"%s\", using default.\n", filename);
    file_type = HTTP_HDR_DEFAULT_TYPE;
  } else {
    file_type = NULL;
    for (j = 0; j < NUM_HTTP_HEADERS; j++) {
      if (!strcmp(file_ext, g_psHTTPHeaders[j].extension)) {
        file_type = g_psHTTPHeaders[j].content_type;
        break;
      }
    }
    if (file_type == NULL) {
      printf("failed to get file type for extension \"%s\", using default.\n", file_ext);
      file_type = HTTP_HDR_DEFAULT_TYPE;
    }
  }

  /* Content-Length is used for persistent connections in HTTP/1.1 but also for
     download progress in older versions
     @todo: just use a big-enough buffer and let the HTTPD send spaces? */
  if (provide_content_len) {
    char intbuf[MAX_PATH_LEN];
    int content_len = file_size;
    memset(intbuf, 0, sizeof(intbuf));
    cur_string = g_psHTTPHeaderStrings[HTTP_HDR_CONTENT_LENGTH];
    cur_len = strlen(cur_string);
    fprintf(data_file, NEWLINE "/* \"%s%d\r\n\" (%"SZT_F"+ bytes) */" NEWLINE, cur_string, content_len, cur_len + 2);
    written += file_put_ascii(data_file, cur_string, cur_len, &i);
    if (precalcChksum) {
      memcpy(&hdr_buf[hdr_len], cur_string, cur_len);
      hdr_len += cur_len;
    }

    lwip_itoa(intbuf, sizeof(intbuf), content_len);
    strcat(intbuf, "\r\n");
    cur_len = strlen(intbuf);
    written += file_put_ascii(data_file, intbuf, cur_len, &i);
    i = 0;
    if (precalcChksum) {
      memcpy(&hdr_buf[hdr_len], intbuf, cur_len);
      hdr_len += cur_len;
    }
  }
  if (provide_last_modified) {
    char modbuf[256];
    struct stat stat_data;
    struct tm *t;
    memset(modbuf, 0, sizeof(modbuf));
    memset(&stat_data, 0, sizeof(stat_data));
    cur_string = modbuf;
    strcpy(modbuf, "Last-Modified: ");
    if (stat(filename, &stat_data) != 0) {
      printf("stat(%s) failed with error %d\n", filename, errno);
      exit(-1);
    }
    t = gmtime(&stat_data.st_mtime);
    if (t == NULL) {
      printf("gmtime() failed with error %d\n", errno);
      exit(-1);
    }
    strftime(&modbuf[15], sizeof(modbuf) - 15, "%a, %d %b %Y %H:%M:%S GMT", t);
    cur_len = strlen(cur_string);
    fprintf(data_file, NEWLINE "/* \"%s\"\r\n\" (%"SZT_F"+ bytes) */" NEWLINE, cur_string, cur_len + 2);
    written += file_put_ascii(data_file, cur_string, cur_len, &i);
    if (precalcChksum) {
      memcpy(&hdr_buf[hdr_len], cur_string, cur_len);
      hdr_len += cur_len;
    }

    modbuf[0] = 0;
    strcat(modbuf, "\r\n");
    cur_len = strlen(modbuf);
    written += file_put_ascii(data_file, modbuf, cur_len, &i);
    i = 0;
    if (precalcChksum) {
      memcpy(&hdr_buf[hdr_len], modbuf, cur_len);
      hdr_len += cur_len;
    }
  }

  /* HTTP/1.1 implements persistent connections */
  if (useHttp11) {
    if (provide_content_len) {
      cur_string = g_psHTTPHeaderStrings[HTTP_HDR_CONN_KEEPALIVE];
    } else {
      /* no Content-Length available, so a persistent connection is no possible
         because the client does not know the data length */
      cur_string = g_psHTTPHeaderStrings[HTTP_HDR_CONN_CLOSE];
    }
    cur_len = strlen(cur_string);
    fprintf(data_file, NEWLINE "/* \"%s\" (%"SZT_F" bytes) */" NEWLINE, cur_string, cur_len);
    written += file_put_ascii(data_file, cur_string, cur_len, &i);
    i = 0;
    if (precalcChksum) {
      memcpy(&hdr_buf[hdr_len], cur_string, cur_len);
      hdr_len += cur_len;
    }
  }

#if MAKEFS_SUPPORT_DEFLATE
  if (is_compressed) {
    /* tell the client about the deflate encoding */
    LWIP_ASSERT("error", deflateNonSsiFiles);
    cur_string = "Content-Encoding: deflate\r\n";
    cur_len = strlen(cur_string);
    fprintf(data_file, NEWLINE "/* \"%s\" (%d bytes) */" NEWLINE, cur_string, cur_len);
    written += file_put_ascii(data_file, cur_string, cur_len, &i);
    i = 0;
  }
#else
  LWIP_UNUSED_ARG(is_compressed);
#endif

  /* write content-type, ATTENTION: this includes the double-CRLF! */
  cur_string = file_type;
  cur_len = strlen(cur_string);
  fprintf(data_file, NEWLINE "/* \"%s\" (%"SZT_F" bytes) */" NEWLINE, cur_string, cur_len);
  written += file_put_ascii(data_file, cur_string, cur_len, &i);
  i = 0;

  /* ATTENTION: headers are done now (double-CRLF has been written!) */

  if (precalcChksum) {
    LWIP_ASSERT("hdr_len + cur_len <= sizeof(hdr_buf)", hdr_len + cur_len <= sizeof(hdr_buf));
    memcpy(&hdr_buf[hdr_len], cur_string, cur_len);
    hdr_len += cur_len;

    LWIP_ASSERT("strlen(hdr_buf) == hdr_len", strlen(hdr_buf) == hdr_len);
    acc = ~inet_chksum(hdr_buf, (u16_t)hdr_len);
    *http_hdr_len = (u16_t)hdr_len;
    *http_hdr_chksum = acc;
  }

  return written;
}

int file_put_ascii(FILE *file, const char *ascii_string, int len, int *i)
{
  int x;
  for (x = 0; x < len; x++) {
    unsigned char cur = ascii_string[x];
    fprintf(file, "0x%02x,", cur);
    if ((++(*i) % HEX_BYTES_PER_LINE) == 0) {
      fprintf(file, NEWLINE);
    }
  }
  return len;
}

int s_put_ascii(char *buf, const char *ascii_string, int len, int *i)
{
  int x;
  int idx = 0;
  for (x = 0; x < len; x++) {
    unsigned char cur = ascii_string[x];
    sprintf(&buf[idx], "0x%02x,", cur);
    idx += 5;
    if ((++(*i) % HEX_BYTES_PER_LINE) == 0) {
      sprintf(&buf[idx], NEWLINE);
      idx += NEWLINE_LEN;
    }
  }
  return len;
}
