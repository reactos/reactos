/*
 * isohybrid.c: Post process an ISO 9660 image generated with mkisofs or
 * genisoimage to allow - hybrid booting - as a CD-ROM or as a hard
 * disk.
 *
 * This is based on the original Perl script written by H. Peter Anvin. The
 * rewrite in C is to avoid dependency on Perl on a system under installation.
 *
 * Copyright (C) 2010 P J P <pj.pandit@yahoo.co.in>
 *
 * isohybrid is a free software; you can redistribute it and/or modify it
 * under the terms of GNU General Public License as published by Free Software
 * Foundation; either version 2 of the license, or (at your option) any later
 * version.
 *
 * isohybrid is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with isohybrid; if not, see: <http://www.gnu.org/licenses>.
 *
 */

#define _FILE_OFFSET_BITS 64
//#include <err.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
//#include <alloca.h>
//#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>
#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
#include <uuid/uuid.h>
#endif

#include "isohybrid.h"
#include "reactos_support_code.h"

char *prog = NULL;
extern int opterr, optind;
struct stat isostat;
unsigned int padding = 0;

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
uuid_t disk_uuid, part_uuid, iso_uuid;
#endif

uint8_t mode = 0;
enum { VERBOSE = 1 , EFI = 2 , MAC = 4};

/* user options */
uint16_t head = 64;             /* 1 <= head <= 256 */
uint8_t sector = 32;            /* 1 <= sector <= 63  */

uint8_t entry = 0;              /* partition number: 1 <= entry <= 4 */
uint8_t offset = 0;             /* partition offset: 0 <= offset <= 64 */
uint16_t type = 0x17;           /* partition type: 0 <= type <= 255 */
uint32_t id = 0;                /* MBR: 0 <= id <= 0xFFFFFFFF(4294967296) */

uint8_t hd0 = 0;                /* 0 <= hd0 <= 2 */
uint8_t partok = 0;             /* 0 <= partok <= 1 */

char mbr_template_path[1024] = {0};   /* Path to MBR template */

uint16_t ve[16];
uint32_t catoffset = 0;
uint32_t c = 0, cc = 0, cs = 0;

uint32_t psize = 0, isosize = 0;

/* boot catalogue parameters */
uint32_t de_lba = 0;
uint16_t de_seg = 0, de_count = 0, de_mbz2 = 0;
uint8_t de_boot = 0, de_media = 0, de_sys = 0, de_mbz1 = 0;
uint32_t efi_lba = 0, mac_lba = 0;
uint16_t efi_count = 0, mac_count = 0;
uint8_t efi_boot = 0, efi_media = 0, efi_sys = 0;

int apm_parts = 3;

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
uint8_t afp_header[] = { 0x45, 0x52, 0x08, 0x00, 0x00, 0x00, 0x90, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

uuid_t efi_system_partition = {0xC1, 0x2A, 0x73, 0x28, 0xF8, 0x1F, 0x11, 0xD2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B};
uuid_t basic_partition = {0xEB,0xD0,0xA0,0xA2,0xB9,0xE5,0x44,0x33,0x87,0xC0,0x68,0xB6,0xB7,0x26,0x99,0xC7};
uuid_t hfs_partition = {0x48, 0x46, 0x53, 0x00, 0x00, 0x00, 0x11, 0xAA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC};
#endif

uint32_t crc_tab[256] =
{
    0, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

struct iso_primary_descriptor {
    uint8_t ignore [80];
    uint32_t size;
    uint8_t ignore2 [44];
    uint16_t block_size;
};

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
struct gpt_header {
    uint64_t signature;
    uint32_t revision;
    uint32_t headerSize;
    uint32_t headerCRC;
    uint32_t reserved;
    uint64_t currentLBA;
    uint64_t backupLBA;
    uint64_t firstUsableLBA;
    uint64_t lastUsableLBA;
    uuid_t diskGUID;
    uint64_t partitionEntriesLBA;
    uint32_t numParts;
    uint32_t sizeOfPartitionEntries;
    uint32_t partitionEntriesCRC;
    uint8_t reserved2[420];
};

struct gpt_part_header {
    uuid_t partTypeGUID;
    uuid_t partGUID;
    uint64_t firstLBA;
    uint64_t lastLBA;
    uint64_t attributes;
    uint16_t name[36];
};

#define APM_OFFSET 2048

struct apple_part_header {
    uint16_t        signature;      /* expected to be MAC_PARTITION_MAGIC */
    uint16_t        res1;
    uint32_t        map_count;      /* # blocks in partition map */
    uint32_t        start_block;    /* absolute starting block # of partition */
    uint32_t        block_count;    /* number of blocks in partition */
    char            name[32];       /* partition name */
    char            type[32];       /* string type description */
    uint32_t        data_start;     /* rel block # of first data block */
    uint32_t        data_count;     /* number of data blocks */
    uint32_t        status;         /* partition status bits */
    uint32_t        boot_start;
    uint32_t        boot_count;
    uint32_t        boot_load;
    uint32_t        boot_load2;
    uint32_t        boot_entry;
    uint32_t        boot_entry2;
    uint32_t        boot_cksum;
    char            processor[16];  /* Contains 680x0, x=0,2,3,4; or empty */
    uint32_t        driver_sig;
    char            _padding[372];
};
#endif


void
usage(void)
{
    printf("Usage: %s [OPTIONS] <boot.iso>\n", prog);
}


void
printh(void)
{
#define FMT "%-20s %s\n"

    usage();

    printf("\n");
    printf("Options:\n");
    printf(FMT, "   -h <X>", "Number of geometry heads (default 64)");
    printf(FMT, "   -s <X>", "Number of geometry sectors (default 32)");
    printf(FMT, "   -e --entry", "Specify partition entry number (1-4)");
    printf(FMT, "   -o --offset", "Specify partition offset (default 0)");
    printf(FMT, "   -t --type", "Specify partition type (default 0x17)");
    printf(FMT, "   -i --id", "Specify MBR ID (default random)");
#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
    printf(FMT, "   -u --uefi", "Build EFI bootable image");
    printf(FMT, "   -m --mac", "Add AFP table support");
#endif
    printf(FMT, "   -b --mbr <PATH>", "Load MBR from PATH");

    printf("\n");
    printf(FMT, "   --forcehd0", "Assume we are loaded as disk ID 0");
    printf(FMT, "   --ctrlhd0", "Assume disk ID 0 if the Ctrl key is pressed");
    printf(FMT, "   --partok", "Allow booting from within a partition");

    printf("\n");
    printf(FMT, "   -? --help", "Display this help");
    printf(FMT, "   -v --verbose", "Display verbose output");
    printf(FMT, "   -V --version", "Display version information");

    printf("\n");
    printf("Report bugs to <pj.pandit@yahoo.co.in>\n");
}


int
check_option(int argc, char *argv[])
{
    char *err = NULL;
    int n = 0, ind = 0;

    const char optstr[] = ":h:s:e:o:t:i:b:umfcp?vV";
    struct option lopt[] = \
    {
        { "entry", required_argument, NULL, 'e' },
        { "offset", required_argument, NULL, 'o' },
        { "type", required_argument, NULL, 't' },
        { "id", required_argument, NULL, 'i' },

        { "forcehd0", no_argument, NULL, 'f' },
        { "ctrlhd0", no_argument, NULL, 'c' },
        { "partok", no_argument, NULL, 'p'},
#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
	{ "uefi", no_argument, NULL, 'u'},
	{ "mac", no_argument, NULL, 'm'},
#endif
        { "mbr", required_argument, NULL, 'b' },

        { "help", no_argument, NULL, '?' },
        { "verbose", no_argument, NULL, 'v' },
        { "version", no_argument, NULL, 'V' },

        { 0, 0, 0, 0 }
    };

    opterr = mode = 0;
    while ((n = getopt_long_only(argc, argv, optstr, lopt, &ind)) != -1)
    {
        switch (n)
        {
        case 'h':
            head = strtoul(optarg, &err, 0);
            if (head < 1 || head > 256)
                errx(1, "invalid head: `%s', 1 <= head <= 256", optarg);
            break;

        case 's':
            sector = strtoul(optarg, &err, 0);
            if (sector < 1 || sector > 63)
                errx(1, "invalid sector: `%s', 1 <= sector <= 63", optarg);
            break;

        case 'e':
            entry = strtoul(optarg, &err, 0);
            if (entry < 1 || entry > 4)
                errx(1, "invalid entry: `%s', 1 <= entry <= 4", optarg);
	    if (mode & MAC || mode & EFI)
		errx(1, "setting an entry is unsupported with EFI or Mac");
            break;

        case 'o':
            offset = strtoul(optarg, &err, 0);
            if (*err || offset > 64)
                errx(1, "invalid offset: `%s', 0 <= offset <= 64", optarg);
            break;

        case 't':
            type = strtoul(optarg, &err, 0);
            if (*err || type > 255)
                errx(1, "invalid type: `%s', 0 <= type <= 255", optarg);
            break;

        case 'i':
            id = strtoul(optarg, &err, 0);
            if (*err)
                errx(1, "invalid id: `%s'", optarg);
            break;

        case 'f':
            hd0 = 1;
            break;

        case 'c':
            hd0 = 2;
            break;

        case 'p':
            partok = 1;
            break;

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
	case 'u':
	    mode |= EFI;
	    if (entry)
		errx(1, "setting an entry is unsupported with EFI or Mac");
	    break;

	case 'm':
	    mode |= MAC;
	    if (entry)
		errx(1, "setting an entry is unsupported with EFI or Mac");
	    break;
#endif

	case 'b':
            if (strlen(optarg) >= sizeof(mbr_template_path))
                errx(1, "--mbr : Path too long");
            strcpy(mbr_template_path, optarg);
            break;

        case 'v':
            mode |= VERBOSE;
            break;

        case 'V':
            printf("%s version %s\n", prog, VERSION);
            exit(0);

        case ':':
            errx(1, "option `-%c' takes an argument", optopt);

        default:
        case '?':
            if (optopt)
                errx(1, "invalid option `-%c', see --help", optopt);

            printh();
            exit(0);
        }
    }

    return optind;
}

uint16_t
bendian_short(const uint16_t s)
{
    uint16_t r = 1;

    if (!*(uint8_t *)&r)
        return s;

    r = (s & 0x00FF) << 8 | (s & 0xFF00) >> 8;

    return r;
}


uint32_t
bendian_int(const uint32_t s)
{
    uint32_t r = 1;

    if (!*(uint8_t *)&r)
        return s;

    r = (s & 0x000000FF) << 24 | (s & 0xFF000000) >> 24
        | (s & 0x0000FF00) << 8 | (s & 0x00FF0000) >> 8;

    return r;
}

uint16_t
lendian_short(const uint16_t s)
{
    uint16_t r = 1;

    if (*(uint8_t *)&r)
        return s;

    r = (s & 0x00FF) << 8 | (s & 0xFF00) >> 8;

    return r;
}


uint32_t
lendian_int(const uint32_t s)
{
    uint32_t r = 1;

    if (*(uint8_t *)&r)
        return s;

    r = (s & 0x000000FF) << 24 | (s & 0xFF000000) >> 24
        | (s & 0x0000FF00) << 8 | (s & 0x00FF0000) >> 8;

    return r;
}

uint64_t
lendian_64(const uint64_t s)
{
	uint64_t r = 1;

	if (*(uint8_t *)&r)
		return s;

       r = (s & 0x00000000000000FFull) << 56 | (s & 0xFF00000000000000ull) >> 56
            | (s & 0x000000000000FF00ull) << 40 | (s & 0x00FF000000000000ull) >> 40
            | (s & 0x0000000000FF0000ull) << 24 | (s & 0x0000FF0000000000ull) >> 24
            | (s & 0x00000000FF000000ull) << 8 | (s & 0x000000FF00000000ull) >> 8;

	return r;
}


int
check_banner(const uint8_t *buf)
{
    static const char banner[] = "\0CD001\1EL TORITO SPECIFICATION\0\0\0\0" \
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" \
        "\0\0\0\0\0";

    if (!buf || memcmp(buf, banner, sizeof(banner) - 1))
        return 1;

    buf += sizeof(banner) - 1;
    memcpy(&catoffset, buf, sizeof(catoffset));

    catoffset = lendian_int(catoffset);

    return 0;
}


int
check_catalogue(const uint8_t *buf)
{
    int i = 0;

    for (i = 0, cs = 0; i < 16; i++)
    {
        ve[i] = 0;
        memcpy(&ve[i], buf, sizeof(ve[i]));

        ve[i] = lendian_short(ve[i]);

        buf += 2;
        cs += ve[i];

        if (mode & VERBOSE)
            printf("ve[%d]: %d, cs: %d\n", i, ve[i], cs);
    }
    if ((ve[0] != 0x0001) || (ve[15] != 0xAA55) || (cs & 0xFFFF))
        return 1;

    return 0;
}


int
read_catalogue(const uint8_t *buf)
{
    memcpy(&de_boot, buf++, 1);
    memcpy(&de_media, buf++, 1);

    memcpy(&de_seg, buf, 2);
    de_seg = lendian_short(de_seg);
    buf += 2;

    memcpy(&de_sys, buf++, 1);
    memcpy(&de_mbz1, buf++, 1);

    memcpy(&de_count, buf, 2);
    de_count = lendian_short(de_count);
    buf += 2;

    memcpy(&de_lba, buf, 4);
    de_lba = lendian_int(de_lba);
    buf += 4;

    memcpy(&de_mbz2, buf, 2);
    de_mbz2 = lendian_short(de_mbz2);
    buf += 2;

    if (de_boot != 0x88 || de_media != 0
        || (de_seg != 0 && de_seg != 0x7C0) || de_count != 4)
        return 1;

    return 0;
}


#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
int
read_efi_section(const uint8_t *buf)
{
	unsigned char header_indicator;
	unsigned char platform_id;
	short count;

	memcpy(&header_indicator, buf++, 1);
	memcpy(&platform_id, buf++, 1);

	memcpy(&count, buf, 2);
	count = lendian_short(count);
	buf += 2;

	if (platform_id == 0xef)
		return 0;

	return 1;
}

int
read_efi_catalogue(const uint8_t *buf, uint16_t *count, uint32_t *lba)
{
    buf += 6;

    memcpy(count, buf, 2);
    *count = lendian_short(*count);
    buf += 2;

    memcpy(lba, buf, 4);
    *lba = lendian_int(*lba);
    buf += 6;

    return 0;
}
#endif


void
display_catalogue(void)
{
    printf("de_boot: %hhu\n", de_boot);
    printf("de_media: %hhu\n", de_media);
    printf("de_seg: %hu\n", de_seg);
    printf("de_sys: %hhu\n", de_sys);
    printf("de_mbz1: %hhu\n", de_mbz1);
    printf("de_count: %hu\n", de_count);
    printf("de_lba: %u\n", de_lba);
    printf("de_mbz2: %hu\n", de_mbz2);
}


void
read_mbr_template(char *path, uint8_t *mbr)
{
    FILE *fp;
    int ret;

    fp = fopen(path, "rb");
    if (fp == NULL)
        err(1, "could not open MBR template file `%s'", path);
    clearerr(fp);
    ret = fread(mbr, 1, MBRSIZE, fp);
    if (ferror(fp) || ret != MBRSIZE)
        err(1, "error while reading MBR template file `%s'", path);
    fclose(fp);
}


int
initialise_mbr(uint8_t *mbr)
{
    int i = 0;
    uint32_t tmp = 0;
    uint8_t ptype = 0, *rbm = mbr;
    uint8_t bhead = 0, bsect = 0, bcyle = 0;
    uint8_t ehead = 0, esect = 0, ecyle = 0;

#ifndef ISOHYBRID_C_STANDALONE
    extern unsigned char isohdpfx[][MBRSIZE];
#endif

    if (mbr_template_path[0]) {
        read_mbr_template(mbr_template_path, mbr);
    } else {

#ifdef ISOHYBRID_C_STANDALONE

        err(1, "This is a standalone binary. You must specify --mbr. E.g with /usr/lib/syslinux/isohdpfx.bin");

#else

        memcpy(mbr, &isohdpfx[hd0 + 3 * partok], MBRSIZE);

#endif /* ! ISOHYBRID_C_STANDALONE */

    }

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
    if (mode & MAC) {
	memcpy(mbr, afp_header, sizeof(afp_header));
    }
#endif

    if (!entry)
	entry = 1;

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
    if (mode & EFI)
	type = 0;
#endif

    mbr += MBRSIZE;                                 /* offset 432 */

    tmp = lendian_int(de_lba * 4);
    memcpy(mbr, &tmp, sizeof(tmp));
    mbr += sizeof(tmp);                             /* offset 436 */

    tmp = 0;
    memcpy(mbr, &tmp, sizeof(tmp));
    mbr += sizeof(tmp);                             /* offset 440 */

    tmp = lendian_int(id);
    memcpy(mbr, &tmp, sizeof(tmp));
    mbr += sizeof(tmp);                             /* offset 444 */

    mbr[0] = '\0';
    mbr[1] = '\0';
    mbr += 2;                                       /* offset 446 */

    ptype = type;
    psize = c * head * sector - offset;

    bhead = (offset / sector) % head;
    bsect = (offset % sector) + 1;
    bcyle = offset / (head * sector);

    bsect += (bcyle & 0x300) >> 2;
    bcyle  &= 0xFF;

    ehead = head - 1;
    esect = sector + (((cc - 1) & 0x300) >> 2);
    ecyle = (cc - 1) & 0xFF;

    for (i = 1; i <= 4; i++)
    {
        memset(mbr, 0, 16);
        if (i == entry)
        {
            mbr[0] = 0x80;
            mbr[1] = bhead;
            mbr[2] = bsect;
            mbr[3] = bcyle;
            mbr[4] = ptype;
            mbr[5] = ehead;
            mbr[6] = esect;
            mbr[7] = ecyle;

            tmp = lendian_int(offset);
            memcpy(&mbr[8], &tmp, sizeof(tmp));

            tmp = lendian_int(psize);
            memcpy(&mbr[12], &tmp, sizeof(tmp));
        }
#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
        if (i == 2 && (mode & EFI))
        {
            mbr[0] = 0x0;
            mbr[1] = 0xfe;
            mbr[2] = 0xff;
            mbr[3] = 0xff;
            mbr[4] = 0xef;
            mbr[5] = 0xfe;
            mbr[6] = 0xff;
            mbr[7] = 0xff;

            tmp = lendian_int(efi_lba * 4);
            memcpy(&mbr[8], &tmp, sizeof(tmp));

            tmp = lendian_int(efi_count);
            memcpy(&mbr[12], &tmp, sizeof(tmp));
        }
        if (i == 3 && (mode & MAC))
        {
            mbr[0] = 0x0;
            mbr[1] = 0xfe;
            mbr[2] = 0xff;
            mbr[3] = 0xff;
            mbr[4] = 0x0;
            mbr[5] = 0xfe;
            mbr[6] = 0xff;
            mbr[7] = 0xff;

            tmp = lendian_int(mac_lba * 4);
            memcpy(&mbr[8], &tmp, sizeof(tmp));

            tmp = lendian_int(mac_count);
            memcpy(&mbr[12], &tmp, sizeof(tmp));
        }
#endif
        mbr += 16;
    }
    mbr[0] = 0x55;
    mbr[1] = 0xAA;
    mbr += 2;

    return mbr - rbm;
}

void
display_mbr(const uint8_t *mbr, size_t len)
{
    unsigned char c = 0;
    unsigned int i = 0, j = 0;

    printf("sizeof(MBR): %zu bytes\n", len);
    for (i = 0; i < len; i++)
    {
        if (!(i % 16))
            printf("%04d ", i);

        if (!(i % 8))
            printf(" ");

        c = mbr[i];
        printf("%02x ", c);

        if (!((i + 1) % 16))
        {
            printf(" |");
            for (; j <= i; j++)
                printf("%c", isprint(mbr[j]) ? mbr[j] : '.');
            printf("|\n");
        }
    }
}


uint32_t chksum_crc32 (unsigned char *block, unsigned int length)
{
	register unsigned long crc;
	unsigned long i;

	crc = 0xFFFFFFFF;
	for (i = 0; i < length; i++)
	{
		crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
	}
	return (crc ^ 0xFFFFFFFF);
}

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
void
reverse_uuid(uuid_t uuid)
{
	uint8_t t, *p = (uint8_t *)uuid;

	t = p[0]; p[0] = p[3]; p[3] = t;
	t = p[1]; p[1] = p[2]; p[2] = t;
	t = p[4]; p[4] = p[5]; p[5] = t;
	t = p[6]; p[6] = p[7]; p[7] = t;
}
#endif

static uint16_t *
ascii_to_utf16le(uint16_t *dst, const char *src)
{
    uint8_t *p = (uint8_t *)dst;
    char c;

    do {
	c = *src++;
	*p++ = c;
	*p++ = 0;
    } while (c);

    return (uint16_t *)p;
}

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
void
initialise_gpt(uint8_t *gpt, uint32_t current, uint32_t alternate, int primary)
{
    struct gpt_header *header = (struct gpt_header *)gpt;
    struct gpt_part_header *part;
    int hole = 0;
    int gptsize = 128 / 4 + 2;

    if (mac_lba) {
	/* 2048 bytes per partition, plus round to 2048 boundary */
	hole = (apm_parts * 4) + 2;
    }

    if (primary) {
	uuid_generate(disk_uuid);
	reverse_uuid(disk_uuid);
    }

    header->signature = lendian_64(0x5452415020494645ull);
    header->revision = lendian_int(0x010000);
    header->headerSize = lendian_int(0x5c);
    header->currentLBA = lendian_64(current);
    header->backupLBA = lendian_64(alternate);
    header->firstUsableLBA = lendian_64(gptsize + hole);
    header->lastUsableLBA = lendian_64((isostat.st_size + padding)/512 -
				       gptsize);
    if (primary)
	header->partitionEntriesLBA = lendian_64(0x02 + hole);
    else
	header->partitionEntriesLBA = lendian_64(current - (128 / 4));
    header->numParts = lendian_int(0x80);
    header->sizeOfPartitionEntries = lendian_int(0x80);
    memcpy(header->diskGUID, disk_uuid, sizeof(uuid_t));

    if (primary)
	gpt += sizeof(struct gpt_header) + hole * 512;
    else
	gpt -= header->sizeOfPartitionEntries * header->numParts;

    part = (struct gpt_part_header *)gpt;
    if (primary) {
	uuid_generate(part_uuid);
	uuid_generate(iso_uuid);
	reverse_uuid(part_uuid);
	reverse_uuid(iso_uuid);
    }

    memcpy(part->partGUID, iso_uuid, sizeof(uuid_t));
    memcpy(part->partTypeGUID, basic_partition, sizeof(uuid_t));
    part->firstLBA = lendian_64(0);
    part->lastLBA = lendian_64(psize - 1);
    ascii_to_utf16le(part->name, "ISOHybrid ISO");

    gpt += sizeof(struct gpt_part_header);
    part++;

    memcpy(part->partGUID, part_uuid, sizeof(uuid_t));
    memcpy(part->partTypeGUID, basic_partition, sizeof(uuid_t));
    part->firstLBA = lendian_64(efi_lba * 4);
    part->lastLBA = lendian_64(part->firstLBA + efi_count - 1);
    ascii_to_utf16le(part->name, "ISOHybrid");

    gpt += sizeof(struct gpt_part_header);

    if (mac_lba) {
	gpt += sizeof(struct gpt_part_header);

	part++;

	memcpy(part->partGUID, part_uuid, sizeof(uuid_t));
	memcpy(part->partTypeGUID, hfs_partition, sizeof(uuid_t));
	part->firstLBA = lendian_64(mac_lba * 4);
	part->lastLBA = lendian_64(part->firstLBA + mac_count - 1);
	ascii_to_utf16le(part->name, "ISOHybrid");

	part--;
    }

    part--;

    header->partitionEntriesCRC = lendian_int (chksum_crc32((uint8_t *)part,
			   header->numParts * header->sizeOfPartitionEntries));

    header->headerCRC = lendian_int(chksum_crc32((uint8_t *)header,
						 header->headerSize));
}

void
initialise_apm(uint8_t *gpt, uint32_t start)
{
    struct apple_part_header *part = (struct apple_part_header *)gpt;

    part->signature = bendian_short(0x504d);
    part->map_count = bendian_int(apm_parts);
    part->start_block = bendian_int(1);
    part->block_count = bendian_int(4);
    strcpy(part->name, "Apple");
    strcpy(part->type, "Apple_partition_map");
    part->data_start = bendian_int(0);
    part->data_count = bendian_int(10);
    part->status = bendian_int(0x03);

    part = (struct apple_part_header *)(gpt + 2048);

    part->signature = bendian_short(0x504d);
    part->map_count = bendian_int(3);
    part->start_block = bendian_int(efi_lba);
    part->block_count = bendian_int(efi_count / 4);
    strcpy(part->name, "EFI");
    strcpy(part->type, "Apple_HFS");
    part->data_start = bendian_int(0);
    part->data_count = bendian_int(efi_count / 4);
    part->status = bendian_int(0x33);

    part = (struct apple_part_header *)(gpt + 4096);

    if (mac_lba)
    {
	part->signature = bendian_short(0x504d);
	part->map_count = bendian_int(3);
	part->start_block = bendian_int(mac_lba);
	part->block_count = bendian_int(mac_count / 4);
	strcpy(part->name, "EFI");
	strcpy(part->type, "Apple_HFS");
	part->data_start = bendian_int(0);
	part->data_count = bendian_int(mac_count / 4);
	part->status = bendian_int(0x33);
    } else {
	part->signature = bendian_short(0x504d);
	part->map_count = bendian_int(3);
	part->start_block = bendian_int((start/2048) + 10);
	part->block_count = bendian_int(efi_lba - start/2048 - 10);
	strcpy(part->name, "ISO");
	strcpy(part->type, "Apple_Free");
	part->data_start = bendian_int(0);
	part->data_count = bendian_int(efi_lba - start/2048 - 10);
	part->status = bendian_int(0x01);
    }
}
#endif

int
main(int argc, char *argv[])
{
    int i = 0;
    FILE *fp = NULL;
    uint8_t *buf = NULL, *bufz = NULL;
    int cylsize = 0, frac = 0;
    size_t orig_gpt_size, free_space, gpt_size;
    struct iso_primary_descriptor descriptor;

    prog = strcpy(alloca(strlen(argv[0]) + 1), argv[0]);
    i = check_option(argc, argv);
    argc -= i;
    argv += i;

    if (!argc)
    {
        usage();
        return 1;
    }

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
    if ((mode & EFI) && offset)
	errx(1, "%s: --offset is invalid with UEFI images\n", argv[0]);
#endif

    srand(time(NULL) << (getppid() << getpid()));

    if (!(fp = fopen(argv[0], "rb+")))
        err(1, "could not open file `%s'", argv[0]);

    if (fseeko(fp, (off_t) (16 << 11), SEEK_SET))
        err(1, "%s: seek error - 0", argv[0]);

    if (fread(&descriptor, sizeof(char), sizeof(descriptor), fp) != sizeof(descriptor))
        err(1, "%s: read error - 0", argv[0]);

    if (fseeko(fp, (off_t) 17 * 2048, SEEK_SET))
        err(1, "%s: seek error - 1", argv[0]);

    bufz = buf = calloc(BUFSIZE, sizeof(char));
    if (fread(buf, sizeof(char), BUFSIZE, fp) != BUFSIZE)
        err(1, "%s", argv[0]);

    if (check_banner(buf))
        errx(1, "%s: could not find boot record", argv[0]);

    if (mode & VERBOSE)
        printf("catalogue offset: %d\n", catoffset);

    if (fseeko(fp, ((off_t) catoffset) * 2048, SEEK_SET))
        err(1, "%s: seek error - 2", argv[0]);

    buf = bufz;
    memset(buf, 0, BUFSIZE);
    if (fread(buf, sizeof(char), BUFSIZE, fp) != BUFSIZE)
        err(1, "%s", argv[0]);

    if (check_catalogue(buf))
        errx(1, "%s: invalid boot catalogue", argv[0]);

    buf += sizeof(ve);
    if (read_catalogue(buf))
        errx(1, "%s: unexpected boot catalogue parameters", argv[0]);

    if (mode & VERBOSE)
        display_catalogue();

    buf += 32;

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
    if (mode & EFI)
    {
	if (!read_efi_section(buf)) {
	    buf += 32;
	    if (!read_efi_catalogue(buf, &efi_count, &efi_lba) && efi_lba) {
		offset = 0;
	    } else {
		errx(1, "%s: invalid efi catalogue", argv[0]);
	    }
	} else {
	    errx(1, "%s: unable to find efi image", argv[0]);
	}
    }
#endif

    buf += 32;

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
    if (mode & MAC)
    {
	if (!read_efi_section(buf)) {
	    buf += 32;
	    if (!read_efi_catalogue(buf, &mac_count, &mac_lba) && mac_lba) {
		offset = 0;
	    } else {
		errx(1, "%s: invalid efi catalogue", argv[0]);
	    }
	} else {
	    errx(1, "%s: unable to find mac efi image", argv[0]);
	}
    }
#endif

    if (fseeko(fp, (((off_t) de_lba) * 2048 + 0x40), SEEK_SET))
        err(1, "%s: seek error - 3", argv[0]);

    buf = bufz;
    memset(buf, 0, BUFSIZE);
    if (fread(buf, sizeof(char), 4, fp) != 4)
        err(1, "%s", argv[0]);

    if (memcmp(buf, "\xFB\xC0\x78\x70", 4))
        errx(1, "%s: boot loader does not have an isolinux.bin hybrid " \
                 "signature. Note that isolinux-debug.bin does not support " \
                 "hybrid booting", argv[0]);

    if (stat(argv[0], &isostat))
        err(1, "%s", argv[0]);

    isosize = lendian_int(descriptor.size) * lendian_short(descriptor.block_size);
    free_space = isostat.st_size - isosize;

    cylsize = head * sector * 512;
    frac = isostat.st_size % cylsize;
    padding = (frac > 0) ? cylsize - frac : 0;

    if (mode & VERBOSE)
        printf("imgsize: %zu, padding: %d\n", (size_t)isostat.st_size, padding);

    cc = c = ( isostat.st_size + padding) / cylsize;
    if (c > 1024)
    {
        warnx("Warning: more than 1024 cylinders: %d", c);
        warnx("Not all BIOSes will be able to boot this device");
        cc = 1024;
    }

    if (!id)
    {
        if (fseeko(fp, (off_t) 440, SEEK_SET))
            err(1, "%s: seek error - 4", argv[0]);

	if (fread(&id, 1, 4, fp) != 4)
	    err(1, "%s: read error", argv[0]);

        id = lendian_int(id);
        if (!id)
        {
            if (mode & VERBOSE)
                printf("random ");
            id = rand();
        }
    }
    if (mode & VERBOSE)
        printf("id: %u\n", id);

    buf = bufz;
    memset(buf, 0, BUFSIZE);
    i = initialise_mbr(buf);

    if (mode & VERBOSE)
        display_mbr(buf, i);

    if (fseeko(fp, (off_t) 0, SEEK_SET))
        err(1, "%s: seek error - 5", argv[0]);

    if (fwrite(buf, sizeof(char), i, fp) != (size_t)i)
        err(1, "%s: write error - 1", argv[0]);

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
    if (efi_lba) {
	reverse_uuid(basic_partition);
	reverse_uuid(hfs_partition);

	/* 512 byte header, 128 entries of 128 bytes */
	orig_gpt_size = gpt_size = 512 + (128 * 128);

	/* Leave space for the APM if necessary */
	if (mac_lba)
	    gpt_size += (4 * 2048);

	buf = calloc(gpt_size, sizeof(char));
	memset(buf, 0, gpt_size);

	/*
	 * We need to ensure that we have enough space for the secondary GPT.
	 * Unlike the primary, this doesn't need a hole for the APM. We still
	 * want to be 1MB aligned so just bump the padding by a megabyte.
	 */
	if (free_space < orig_gpt_size && padding < orig_gpt_size) {
	    padding += 1024 * 1024;
	}

	/*
	 * Determine the size of the ISO filesystem. This will define the size
	 * of the partition that covers it.
	 */
	psize = isosize / 512;

	/*
	 * Primary GPT starts at sector 1, secondary GPT starts at 1 sector
	 * before the end of the image
	 */
	initialise_gpt(buf, 1, (isostat.st_size + padding - 512) / 512, 1);

	if (fseeko(fp, (off_t) 512, SEEK_SET))
	    err(1, "%s: seek error - 6", argv[0]);

	if (fwrite(buf, sizeof(char), gpt_size, fp) != (size_t)gpt_size)
	    err(1, "%s: write error - 2", argv[0]);
    }

    if (mac_lba)
    {
	/* Apple partition entries filling 2048 bytes each */
	int apm_size = apm_parts * 2048;

	buf = realloc(buf, apm_size);
	memset(buf, 0, apm_size);

	initialise_apm(buf, APM_OFFSET);

	fseeko(fp, (off_t) APM_OFFSET, SEEK_SET);
	fwrite(buf, sizeof(char), apm_size, fp);
    }
#endif

    if (padding)
    {
        if (fsync(fileno(fp)))
            err(1, "%s: could not synchronise", argv[0]);

        if (ftruncate(fileno(fp), isostat.st_size + padding))
            err(1, "%s: could not add padding bytes", argv[0]);
    }

#ifdef REACTOS_ISOHYBRID_EFI_MAC_SUPPORT
    if (efi_lba) {
	buf = realloc(buf, orig_gpt_size);
	memset(buf, 0, orig_gpt_size);

	buf += orig_gpt_size - sizeof(struct gpt_header);

	initialise_gpt(buf, (isostat.st_size + padding - 512) / 512, 1, 0);

	/* Shift back far enough to write the 128 GPT entries */
	buf -= 128 * sizeof(struct gpt_part_header);

	/*
	 * Seek far enough back that the gpt header is 512 bytes before the
	 * end of the image
	 */

	if (fseeko(fp, (isostat.st_size + padding) - orig_gpt_size, SEEK_SET))
	    err(1, "%s: seek error - 8", argv[0]);

	if (fwrite(buf, sizeof(char), orig_gpt_size, fp) != orig_gpt_size)
	    err(1, "%s: write error - 4", argv[0]);
    }
#endif

    free(buf);
    fclose(fp);

    return 0;
}
