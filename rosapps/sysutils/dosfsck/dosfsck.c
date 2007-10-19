/* dosfsck.c  -  User interface */

/* Written 1993 by Werner Almesberger */

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */


#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "common.h"
#include "dosfsck.h"
#include "io.h"
#include "boot.h"
#include "fat.h"
#include "file.h"
#include "check.h"


int interactive = 0,list = 0,test = 0,verbose = 0,write_immed = 0;
int atari_format = 0;
unsigned n_files = 0;
void *mem_queue = NULL;


static void usage(char *name)
{
    fprintf(stderr,"usage: %s [-aAflrtvVwy] [-d path -d ...] "
      "[-u path -u ...]\n%15sdevice\n",name,"");
    fprintf(stderr,"  -a       automatically repair the file system\n");
    fprintf(stderr,"  -A       toggle Atari file system format\n");
    fprintf(stderr,"  -d path  drop that file\n");
    fprintf(stderr,"  -f       salvage unused chains to files\n");
    fprintf(stderr,"  -l       list path names\n");
    fprintf(stderr,"  -n       no-op, check non-interactively without changing\n");
    fprintf(stderr,"  -r       interactively repair the file system\n");
    fprintf(stderr,"  -t       test for bad clusters\n");
    fprintf(stderr,"  -u path  try to undelete that (non-directory) file\n");
    fprintf(stderr,"  -v       verbose mode\n");
    fprintf(stderr,"  -V       perform a verification pass\n");
    fprintf(stderr,"  -w       write changes to disk immediately\n");
    fprintf(stderr,"  -y       same as -a, for compat with other *fsck\n");
    exit(2);
}


/*
 * ++roman: On m68k, check if this is an Atari; if yes, turn on Atari variant
 * of MS-DOS filesystem by default.
 */
static void check_atari( void )
{
#ifdef __mc68000__
    FILE *f;
    char line[128], *p;

    if (!(f = fopen( "/proc/hardware", "r" ))) {
	perror( "/proc/hardware" );
	return;
    }

    while( fgets( line, sizeof(line), f ) ) {
	if (strncmp( line, "Model:", 6 ) == 0) {
	    p = line + 6;
	    p += strspn( p, " \t" );
	    if (strncmp( p, "Atari ", 6 ) == 0)
		atari_format = 1;
	    break;
	}
    }
    fclose( f );
#endif
}


int main(int argc,char **argv)
{
    DOS_FS fs;
    int rw,salvage_files,verify,c;
    unsigned long free_clusters;

    rw = salvage_files = verify = 0;
    interactive = 1;
    check_atari();

    while ((c = getopt(argc,argv,"Aad:flnrtu:vVwy")) != EOF)
	switch (c) {
	    case 'A': /* toggle Atari format */
	  	atari_format = !atari_format;
		break;
	    case 'a':
	    case 'y':
		rw = 1;
		interactive = 0;
		salvage_files = 1;
		break;
	    case 'd':
		file_add(optarg,fdt_drop);
		break;
	    case 'f':
		salvage_files = 1;
		break;
	    case 'l':
		list = 1;
		break;
	    case 'n':
		rw = 0;
		interactive = 0;
		break;
	    case 'r':
		rw = 1;
		interactive = 1;
		break;
	    case 't':
		test = 1;
		break;
	    case 'u':
		file_add(optarg,fdt_undelete);
		break;
	    case 'v':
		verbose = 1;
		printf("dosfsck " VERSION " (" VERSION_DATE ")\n");
		break;
	    case 'V':
		verify = 1;
		break;
	    case 'w':
		write_immed = 1;
		break;
	    default:
		usage(argv[0]);
	}

    if ((test || write_immed) && !rw) {
	fprintf(stderr,"-t and -w require -a or -r\n");
	exit(2);
    }
    if (optind != argc-1) usage(argv[0]);

    printf( "dosfsck " VERSION ", " VERSION_DATE ", FAT32, LFN\n" );
    fs_open(argv[optind],rw);
    read_boot(&fs);
    if (verify) printf("Starting check/repair pass.\n");
    while (read_fat(&fs), scan_root(&fs)) qfree(&mem_queue);
    if (test) fix_bad(&fs);
    if (salvage_files) reclaim_file(&fs);
    else reclaim_free(&fs);
    free_clusters = update_free(&fs);
    file_unused();
    qfree(&mem_queue);
    if (verify) {
	printf("Starting verification pass.\n");
	read_fat(&fs);
	scan_root(&fs);
	reclaim_free(&fs);
	qfree(&mem_queue);
    }

    if (fs_changed()) {
	if (rw) {
	    if (interactive)
		rw = get_key("yn","Perform changes ? (y/n)") == 'y';
	    else printf("Performing changes.\n");
	}
	else
	    printf("Leaving file system unchanged.\n");
    }

    printf( "%s: %u files, %lu/%lu clusters\n", argv[optind],
	    n_files, fs.clusters - free_clusters, fs.clusters );

    return fs_close(rw) ? 1 : 0;
}

/* Local Variables: */
/* tab-width: 8     */
/* End:             */
