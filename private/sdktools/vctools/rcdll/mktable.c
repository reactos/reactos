/*****************************************************************************
 * mktable - table-building program to ease table maintenance problems
 *
 * DESCRIPTION
 *  Several parts of the FORTRAN compiler need large tables.
 *  For example, the lexer contains tables of keywords and multicharacter
 *  tokens; the intrinsic-function handler contains a table of all the
 *  FORTRAN intrinsic functions.
 *  Maintaining these tables can be aggravating, since they are typically
 *  large and involve lots of drudge work (like changing many sequentially-
 *  numbered macro definitions) to modify.
 *
 *  `mktable' can be used to build tables automatically as part of the
 *  usual compiler building process.  Its usages and semantics are as
 *  follows.
 *
 *  `mktable' takes a "table" file on its standard input.  Each line of
 *  the table file has one of the following forms:
 *
 *      # commentary information
 *      "key-string" [index-macro-name [arbitrary-stuff]]
 *      <blank line>
 *
 *  The key string and arbitrary-stuff form the contents of a single
 *  table record.  The index-macro-name is #define'd to be the index
 *  of the given record in the table.  If the index-macro-name is absent or
 *  is an empty string ("") then no macro definition is produced for the
 *  record.
 *
 *  `mktable' produces its output on four files:
 *      mktable.keys: the key string
 *      mktable.defs: #define <index_macro_name> <index to mktable.keys>
 *      mktable.indx: contains the initialization part of a definition
 *          for an index array for key-letter indexed tables,
 *          or the initialization part of a collision-resolution
 *          table for linear-list hashed tables.
 *          (not generated for sorted or _open-addressed tables.)
 *      mktable.info: contains arbitrary-stuff
 *
 *  For example, if the table to be defined were named "symtab" and the
 *  table being constructed was of the "sorted" type (suitable for binary
 *  search),
 *
 *      # contents of symtab:
 *      "alpha" ST_ALPHA    2, 4, MONADIC
 *      "gamma" ST_GAMMA    2, 3, MONADIC
 *      "delta" ST_DELTA    2, 1, DYADIC
 *      "epsilon"
 *
 *  then `mktable' produces the following in mktable.keys:
 *
 *      "alpha","delta","epsilon","gamma"
 *
 *  and the following in mktable.defs:
 *
 *      #define ST_ALPHA 0
 *      #define ST_DELTA 1
 *      #define ST_GAMMA 2
 *
 *  and in mktable.info :
 *
 *      {2, 4, MONADIC}, {2, 1, DYADIC}, {0}, {2, 3, MONADIC}
 *
 *  The files might be included in a C source program in the
 *  following way:
 *
 *      #include "mktable.defs"
 *      ...
 *      char    *symname[] = {
 *      #   include "mktable.keys"
 *          };
 *      struct syminfo
 *          {
 *          int size;
 *          int cycles;
 *          int arity;
 *          };
 *      struct syminfo symtab[] = {
 *      #   include "mktable.info"
 *          };
 *
 *  The `mktable' command itself is used in one of the following ways:
 *
 *  mktable "open" size <tablefile
 *      This form creates an _open-addressed hash table, keyed on
 *      the string fields at the beginning of each record in the
 *      table file.  The hash function used is the absolute value
 *      of the sum of all the characters in a key, modulo the table
 *      size.  The collision resolution function is simply one plus
 *      the last hash, modulo the table size.
 *      Since some of the entries in the hash table may be empty,
 *      and `mktable' has no way of knowing how to fill them,
 *      one of the records supplied by the user will be replicated
 *      in the empty entries with its key value set to NULL.
 *      "table.c" will be created with the hash table itself, and
 *      "table.h" will be created with index-macro definitions that
 *      may be used to index directly into the table in "table.c".
 *
 *  mktable "hashed" size <tablefile
 *      This form creates a hash table keyed on the string fields
 *      at the beginning of each table file record.  The hash function
 *      is the absolute value of the sum of all the characters in a
 *      key, modulo the table size.  Collision resolution is handled
 *      with linear chaining, as follows:  If two keys hash to the
 *      same table location, the first one will be placed in the table,
 *      and the corresponding entry of the collision resolution vector
 *      will contain the (integer) index of the next table slot to be
 *      checked for the hash synonym.  When the collision resolution
 *      vector entry is -1, the end of the chain has been reached.
 *      Note that since all entries are stored in the main table, the
 *      `size' must be at least as large as the number of entries.
 *      As with _open addressing, some slots in the table may be
 *      padded with a replicated entry (key value set to NULL).
 *      "table.c" receives the hash table.  "table.h" receives the
 *      index-macro definitions that will index into the table in
 *      "table.c".  "tabindex.c" receives the conflict resolution
 *      vector.
 *
 *  mktable "sorted" <tablefile
 *      This form creates a table sorted in ascending order, keyed
 *      on the string fields at the beginning of each record in the
 *      table file.  Comparisons are ordered according to the ASCII
 *      values of the characters being compared.
 *      "table.c" will be created with the sorted table itself, and
 *      "table.h" will be created with index-macro definitions that
 *      may be used to index directly into the table in "table.c".
 *
 *  mktable "key-letter" <tablefile
 *      This form creates a key-letter-indexed table.
 *      The string fields serve as the
 *      key letter.  An auxiliary table indexed from 'A' to 'Z'+1
 *      gives the starting index of all the entries whose keys begin
 *      with each letter (the last entry duplicates the entry for 'Z').
 *      "table.c" will contain the sorted table.  "tabindex.c" will
 *      contain the auxiliary index table information.  "table.h" will
 *      contain the index-macro definitions that may be used to index
 *      directly into the "table.c" table.
 *      Note that key-letter tables are sorted in a peculiar way;
 *      in ascending order by first letter of the key, but descending
 *      order by the remainder of the key.  This is required by
 *      FORTRAN, to insure that longer keywords are matched before
 *      shorter keywords that are initial substrings of the longer
 *      keywords.
 *      Also note that the key strings themselves are missing the first
 *      char, since by indexing into the table, we are always assured
 *      of having matched the first char.
 *
 * AUTHOR
 *      February, 1984      Allen Akin
 *
 * MODIFICATIONS
 *  March 8, 1984       Allen Akin
 *      Added linear-list resolved hashing.
 *****************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define MAXRECORDS  300     /* maximum-size table we can handle */
#define MAXLINE     82      /* maximum line length (incl "\n\0") */

#define HASHED      0       /* flag used by table loader */
#define LINEAR      1       /* ditto */
#define OPENADDR    2       /* ditto */

#define KEYFILE         "mktable.key"   /* name of table output file */
#define DEFFILE         "mktable.def"   /* name of index defs output file */
#define INDEXFILE       "mktable.ind"   /* name of table index output file */
#define INFOFILE        "mktable.inf"   /* gots the infos in it */

typedef struct rec {
    char *key;      /* key-string field */
    char *id;       /* index macro identifier */
    char *other;    /* other stuff in the record - output untouched */
    struct rec *link;   /* pointer to next record in hash synonyms list */
} Rec_t;

int Upper = 0;

FILE *Fkeys, *Findex, *Fdefs, *Finfo;

/************************************************************************/
/* Function Prototypes                          */
/************************************************************************/
void main (int argc, char **argv);
void usage (void);
void error(char * message);
void open_addr(int size);
void hash_linear(int size);
void sorted(void);
void key_letter(void);
int load(Rec_t *record, int method, int size);
void startoutput(void);
void endoutput(void);
void outrec(Rec_t *rec);
void outdef(char *name, int value);
void outinx(int value);
void sortrec(Rec_t **rptr, int size);
int hash(register char *name);


/************************************************************************/
/* Program code                             */
/************************************************************************/
void  __cdecl
main (
    int argc,
    char **argv
    )
{
    if (argc <= 1)
        usage();

    if(strcmp(argv[1], "-U") == 0) {
        Upper = 1;
        argv++;
        argc--;
    }

    if (strcmp(argv[1], "open") == 0) {
        if (argc != 3)
            usage();
        open_addr(atoi(argv[2]));
    } else if (strcmp(argv[1], "hashed") == 0) {
        if (argc != 3)
            usage();
        hash_linear(atoi(argv[2]));
    } else if (strcmp(argv[1], "sorted") == 0) {
        if (argc != 2)
            usage();
        sorted();
    } else if (strcmp(argv[1], "key-letter") == 0) {
        if (argc != 2)
            usage();
        key_letter();
    } else
        usage();
    exit(0);
}

void
usage (
    void
    )
{
    error("usage: mktable (open SIZE | hashed SIZE | sorted | key-letter) <table-master");
}

void
error(
    char * message
    )
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

void
open_addr(
    int size
    )
{
    register Rec_t *record;     /* points to array storing all records */
    Rec_t defrec;               /* "default" record for empty array slot */
    register int i;

    if (size <= 0)
        error("hash table size specified is less than zero");

    if ((record = (Rec_t *)calloc(size, sizeof(Rec_t))) == NULL)
        error("insufficient memory for hash table");

    for (i = 0; i < size; ++i)
        record[i].key = NULL;

    if (load(record, OPENADDR, size) == 0)
        error("couldn't find any input records");

    defrec.key = NULL;
    defrec.id = NULL;
    for (i = 0; i < size; ++i)
    if (record[i].key != NULL)
        break;
    defrec.other = record[i].other;

    startoutput();

    for (i = 0; i < size; ++i) {
        if (record[i].key == NULL) {
            outrec(&defrec);
        } else {
            outrec(&record[i]);
            outdef(record[i].id, i);
        }
    }

    endoutput();
    _unlink(INDEXFILE);
}

void
hash_linear(
    int size
    )
{
    register Rec_t *record,     /* stores some records, all buckets */
                    *rp;
    Rec_t defrec;               /* default record for empty hash table slots */
    register int i,
                 nextslot,      /* next empty slot in main hash table */
                 prev;

    if (size <= 0)
        error("hash table size specified is less than zero");

    if ((record = (Rec_t *)calloc(size, sizeof(Rec_t))) == NULL)
        error("insufficient memory for hash table");

    for (i = 0; i < size; ++i) {
        record[i].key = NULL;
        record[i].link = NULL;
    }

    if ((i = load(record, HASHED, size)) == 0)
        error("couldn't find any input records");

    if (i > size)
        error("too many records to hold in table");

    defrec.key = NULL;
    defrec.id = NULL;
    for (i = 0; i < size; ++i) {
        if (record[i].key != NULL)
            break;
    }
    defrec.other = record[i].other;
    defrec.link = NULL;
    /*
     * The `load' routine has built a hash table `record'.
     * Each entry in `record' is either empty (key == NULL) or contains a record.
     * Each record may have a NULL link field, or a link field that points to
     * a hash synonym.
     * With this section of code, we rearrange the linked lists of hash synonyms
     * so that all the entries are stored in `record'.
     */
    nextslot = 0;
    for (i = 0; i < size; ++i) {
        if ((record[i].key != NULL) &&
            (record[i].link != NULL) &&
            ((record[i].link < record) || (record[i].link >= (record + size))))
        {
            for (prev = i, rp = record[i].link; rp != NULL; rp = rp->link) {
                while (record[nextslot].key != NULL)
                    ++nextslot;
                record[prev].link = &record[nextslot];
                record[nextslot] = *rp;
                prev = nextslot;
            }
        }
    }

    startoutput();

    for (i = 0; i < size; ++i) {
        if (record[i].key == NULL) {
            outrec(&defrec);
            outinx(-1);
        } else {
            outrec(&record[i]);
            if (record[i].link == NULL)
                outinx(-1);
            else
                outinx(record[i].link - record);    /* cvt. to inx in table */
            outdef(record[i].id, i);
        }
    }

    endoutput();
}

void
sorted(
    void
    )
{
    Rec_t  record[MAXRECORDS],
          *rptr[MAXRECORDS];
    register int i, size;

    size = load(record, LINEAR, MAXRECORDS);

    for (i = 0; i < size; ++i)
        rptr[i] = &record[i];

    sortrec(rptr, size);

    startoutput();

    for (i = 0; i < size; ++i) {
        outrec(rptr[i]);
        outdef(rptr[i]->id, i);
    }

    endoutput();
    _unlink(INDEXFILE);
}

void
key_letter(
    void
    )
{
    Rec_t  record[MAXRECORDS],
          *rptr[MAXRECORDS],
          *temp;
    register int i, size, j, k, l;

    register char lastletter;

    size = load(record, LINEAR, MAXRECORDS);

    for (i = 0; i < size; ++i)
        rptr[i] = &record[i];

    sortrec(rptr, size);

    for (i = 0; i < size; i = j) {
        for (j = i; j < size; ++j) {
            if (rptr[i]->key[0] != rptr[j]->key[0])
                break;
        }

        l = j - 1;

        for (k = i; k < l; ++k, --l) {
            temp = rptr[k];
            rptr[k] = rptr[l];
            rptr[l] = temp;
        }
    }

    startoutput();

    lastletter = (char)((Upper ? 'A' : '_') - 1);
    for (i = 0; i < size; ++i)
    {
        while (rptr[i]->key[0] > lastletter) {
            outinx(i);
            ++lastletter;
        }
        outrec(rptr[i]);
        outdef(rptr[i]->id, i);
    }


    for (; lastletter < (char)((Upper ? 'Z' : 'z') + 1); ++lastletter)
        outinx(size);

    endoutput();
}

int
load(
    Rec_t *record,
    int method,
    int size
    )
{
    char *line;
    register char *p;
    int rec, h, chainlen, maxchainlen = 0, collisions = 0;
    Rec_t r;

    for (rec = 0; ; ++rec)
    {
        if ((line = malloc(MAXLINE)) == NULL)
            error("insufficient memory to load records");

        if (fgets(line, MAXLINE, stdin) == NULL)
            break;

        if (rec >= size)
            error("too many records to handle");

        r.key = r.id = r.other = NULL;
        r.link = NULL;

        for (p = line; *p && isspace(*p); ++p)
            ;
        if (*p != '"') {
            free(line);
            --rec;
            continue;
        }
        r.key = ++p;
        for (; *p != '"'; ++p) {
            if(Upper && (islower(*p)))
                *p = (char)toupper(*p);
        }

        *p++ = '\0';

        for (; *p && isspace(*p); ++p)          /* skip space key and id */
            ;
        if (*p == '"' && *(p + 1) == '"') {     /* no id */
            r.id = NULL;
            p += 2;
        } else if (*p) {
            r.id = p++;                         /* id start */
            for (; *p && ( ! isspace(*p)); ++p) /* til first space */
                ;
            if(*p) {
                *p++ = '\0';                    /* terminate id */
            }
        }

        for (; *p && isspace(*p); ++p)      /* skip space til other info */
            ;
        if(*p) {
            r.other = p++;
            for (; *p != '\n' && *p != '\0'; ++p)
                ;
            *p = '\0';
        }

        if (method == LINEAR) {
            record[rec] = r;
        } else if (method == OPENADDR) {
            chainlen = 0;
            for(h = hash(r.key) % size; record[h].key; h = (h+1) % size) {
                ++chainlen;
                ++collisions;
            }
            maxchainlen = (chainlen < maxchainlen)? maxchainlen: chainlen;
            record[h] = r;
        } else { /* method == HASHED */
            Rec_t  *rp;

            h = hash(r.key) % size;
            if (record[h].key == NULL) {
                record[h] = r;
            } else {
                if ((rp = (Rec_t *)malloc(sizeof(Rec_t))) == NULL)
                    error("insufficient memory to store all records");
                *rp = record[h];
                r.link = rp;
                record[h] = r;
                ++collisions;
                chainlen = 1;
                for (rp = &record[h]; rp->link != NULL; rp = rp->link)
                    ++chainlen;
                maxchainlen = (chainlen < maxchainlen)? maxchainlen: chainlen;
            }
        }
    }

    if (method == HASHED || method == OPENADDR)
        fprintf(stderr, "%d collisions, max chain length %d\n", collisions, maxchainlen);

    return rec;
}

void
startoutput(
    void
    )
{
    if ((Fkeys = fopen(KEYFILE, "w")) == NULL)
        error("can't open keys output file");

    if ((Findex = fopen(INDEXFILE, "w")) == NULL)
        error("can't open index output file");

    if ((Fdefs = fopen(DEFFILE, "w")) == NULL)
        error("can't open definitions output file");

    if ((Finfo = fopen(INFOFILE, "w")) == NULL)
        error("can't open info output file");
}

void
endoutput(
    void
    )
{
    fclose(Fkeys);
    fclose(Findex);
    fclose(Fdefs);
    fclose(Finfo);
}

void outrec(Rec_t *rec)
{
    if (rec->key == NULL)
        fprintf(Fkeys, "NULL,\n");
    else
        fprintf(Fkeys, "\"%s\",\n", ((rec->key) + 1));

    if (rec->other == NULL)
        fprintf(Finfo, "{0},\n");
    else
        fprintf(Finfo, "{%s},\n", rec->other);
}

void
outdef(
    char *name,
    int value
    )
{
    if (name != NULL)
        fprintf(Fdefs, "#define %s %d\n", name, value);
}

void
outinx(
    int value
    )
{
    fprintf(Findex, "%d,\n", value);
}
/*
 * Following code defines the hash function used in `mktable' and in
 * the compiler.  Since we must guarantee they are the same function,
 * we use a single source file.
 *
 * `mktable' does not use the standard include file that the compiler
 * uses, so we define the allowable register declarations here.
 */
#define REG1 register
#define REG2 register
#define REG3 register

void
sortrec(
    Rec_t **rptr,
    int size
    )
{
    register int j, i, gap;
    Rec_t  *temp;

    for (gap = size / 2; gap > 0; gap /= 2) {
        for (i = gap; i < size; ++i) {
            for (j = i - gap; j >= 0; j -= gap) {
                if (strcmp(rptr[j]->key, rptr[j + gap]->key) <= 0)
                    break;
                temp = rptr[j];
                rptr[j] = rptr[j + gap];
                rptr[j + gap] = temp;
            }
        }
    }
}

int
hash(
    register char *name
    )
{
    register    int i;

    i = 0;
    while(*name) {
        i += *name++ ;
    }
    return(i) ;
}
