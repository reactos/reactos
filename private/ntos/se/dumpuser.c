
/*  Defines for Jim   */
// #define LONGNAMES
#define SECPKG       "[Tmpv1_0]"

#ifdef LONGNAMES
#define SERVOP "DOMAIN_SERVER_OPERATORS "
#define ACCTOP "DOMAIN_ACCOUNT_OPERATORS "
#define COMMOP "DOMAIN_COMM_OPERATORS "
#define PRINTOP "DOMAIN_PRINT_OPERATORS "
#define DISKOP "DOMAIN_DISK_OPERATORS "
#define AUDITOP "DOMAIN_AUDIT_OPERATORS "
#define GADMINS  "DOMAIN_ADMIN "
#define GUSERS   "DOMAIN_USERS "
#define GGUESTS  "DOMAIN_GUESTS "
#else
#define SERVOP    "D_SERVER "
#define ACCTOP    "D_ACCOUN "
#define COMMOP    "D_COMM_O "
#define PRINTOP   "D_PRINT_ "
#define DISKOP    "D_DISK_O "
#define AUDITOP   "D_AUDIT_ "
#define GADMINS   "D_ADMIN "
#define GUSERS    "D_USERS "
#define GGUESTS   "D_GUESTS "
#endif


#define INCL_DOSMEMMGR
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netcons.h>
#include <neterr.h>
#include <netlib.h>
#include <uascache.h>
#include <access.h>
#include <permit.h>
#include "ssitools.h"

char FAR *  buf1;
char FAR *  buf2;
char FAR *  GroupCache;
char        path[256];
char        userbuf[MAX_USER_SIZE];
unsigned short    Handle;
struct _ahdr FAR * Header;

void InitMemStuff(void);
void loadit(void);
void showentries(void);
void doargs(int, char**);

struct my_group {
   char  name[GNLEN + 1];
   char  pad;
   unsigned short gid;
   unsigned long  serial;
};

static int
UserHashVal(uname, domain)
const char far *uname;
int domain;
{
        register unsigned val=0;
        register unsigned char c;

        while ((c= (unsigned char) *uname++))
                val += toupper(c);

        return (int) (val % domain);
}

unsigned
dread(unsigned handle, unsigned long pos, char far * buffer, unsigned length)
{
   unsigned short err, bread;
   unsigned long newpos;

   err = DosChgFilePtr(handle, pos, FILE_BEGIN, &newpos);
   if (err)
      return err;
   if (newpos != pos)
      return NERR_ACFFileIOFail;
   err = DosRead( handle, buffer, length, &bread);
   if (err)
      return err;
   if (bread != length)
      return NERR_ACFFileIOFail;
   return 0;
}

unsigned read_object(unsigned handle, unsigned long pos, char FAR * buf)
{
   unsigned err;
   struct disk_obj_hdr FAR * dobj;

   err = dread(handle, pos, buf, sizeof(struct disk_obj_hdr));
   if (err)
      return err;
   dobj = (struct disk_obj_hdr FAR *) buf;
   return dread(handle, pos, buf, dobj->do_numblocks * 64);
}

void InitMemStuff()
{
   unsigned short    err;
   unsigned short    sel, action;

   err = DosAllocSeg(0x8000, &sel, 0);
   if (err) {
      printf("Could not alloc memory, error %d\n", err);
      exit(1);
   }
   buf1 = MAKEP(sel, 0);
   err = DosAllocSeg(0x8000, &sel, 0);
   if (err) {
      printf("Could not alloc memory, error %d\n", err);
      exit(1);
   }
   buf2 = MAKEP(sel, 0);
   err = DosAllocSeg(1024, &sel, 0);
   if (err) {
      printf("Could not alloc memory, error %d\n", err);
      exit(1);
   }
   Header = MAKEP(sel, 0);
   if (err = DosAllocSeg(0x8000, &sel, 0)) {
      printf("Could not alloc memory, error %d\n", err);
      exit(1);
   }
   GroupCache = MAKEP(sel, 0);
   err = DosOpen(path, &Handle, &action, 0L, 0, FILE_OPEN,
                  OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0L);
   if (err) {
      printf("Error opening %s, code %d\n", path, err);
      exit(1);
   }
   err = DosRead(Handle, Header, 512, &action);
   if (err || action != 512) {
      printf("Error reading from file, code %d\n", err);
      exit(1);
   }
   printf("Total users, %d\n", Header->num_users);
}

void loadit()
{
   unsigned short    err;
   unsigned short    action;
   unsigned short    i;
   unsigned long     pos;
   struct diskuserhash FAR *diskhashentry;
   struct userhash FAR *userhashentry;
   struct _grouprec FAR * grec;
   struct my_group FAR * mygroup;

   err = DosChgFilePtr(Handle, 512L, FILE_BEGIN, &pos);
   err = DosRead(Handle, buf1, sizeof(struct _grouprec) * MAXGROUP, &action);
   if (err || action != sizeof(struct _grouprec) * MAXGROUP) {
      printf("Error reading from file, %d\n", err);
      exit(1);
   }
   mygroup = (struct my_group FAR *) GroupCache;
   grec = (struct _grouprec FAR *) buf1;
   for (i = 0; i < MAXGROUP ; i++, grec++, mygroup++ ) {
      if (grec->name[0] != REC_EMPTY && grec->name[0] != REC_DELETE) {
/*         if (strcmpf(grec->name, "ADMINS") == 0) {
            strcpyf(mygroup->name, GADMIN);
          else if (strcmpf(grec->name, "USERS") == 0)
            strcpyf(mygroup->name, GUSERS);
          else if (strcmpf(grec->name, "GUESTS") == 0)
            strcpyf(mygroup->name, GGUESTS);
          else */
         strncpyf(mygroup->name, grec->name, GNLEN);
         mygroup->gid = UserHashVal(mygroup->name, MAXGROUP);
         mygroup->serial = grec->serial;
      } else
         mygroup->name[0] = '\0';
   }

   err = DosChgFilePtr(Handle, (unsigned long) HASH_TBL_OFFSET, FILE_BEGIN, &pos);
   diskhashentry = (struct diskuserhash FAR *)  buf1;
   if (err = DosRead (Handle, diskhashentry, HASH_TBL_SIZE, &action)) {
      printf("Could not read hash table, error %d\n", err);
      exit(1);
   }

   if (action != HASH_TBL_SIZE) {
      printf("Could not read hash table\n");
      exit(1);
   }
   /*
    *  Copy disk user hash table into memory
    */
    userhashentry = (struct userhash FAR *) buf2;
    for (i = 0; i < USER_HASH_ENTRIES; i++) {
                userhashentry->uh_disk = diskhashentry->dh_disk;
                userhashentry->uh_serial = diskhashentry->dh_serial;
                userhashentry->uh_cache = NULL;
                userhashentry++;
                diskhashentry++;
        }
}

void printgroups()
{
   struct my_group FAR * mygroup = (struct my_group FAR *) GroupCache;
   unsigned i;
   unsigned long  relid;

   printf("\t// Groups\n");
   for (i = 0; i< MAXGROUP ; i++, mygroup++ ) {
      if (mygroup->name[0]) {
         relid = ((mygroup->gid | GROUPIDMASK) + (mygroup->serial << 16)) ^ 0x80000000L;
         printf("\tGroup = %ld %Fs\n", relid, mygroup->name);
      }
   }
}
void showentries()
{
   unsigned i, j, k;
   unsigned err;
   unsigned long pos, oprights;
   signed long relativeid;
   unsigned usercount = 0;
   struct userhash FAR * entry = (struct userhash FAR *) buf2;
   struct user_object FAR * uo = (struct user_object FAR *) userbuf;
   struct my_group FAR * mygroup = (struct my_group FAR *) GroupCache;
   unsigned char FAR * gmask;
   unsigned char test;
   char groupnames[256];

   printf("\n\n\t// Users\n");
   for (i = 0; i < USER_HASH_ENTRIES ; i++ ) {
      pos = entry->uh_disk;
      while (pos) {
         if (err = read_object(Handle, pos, userbuf)) {
            printf("Failed reading object, code %d\n", err);
            exit(1);
         }
         relativeid = ( ((uo->uo_record.user.uc0_guid.guid_serial) << 16) +
                      (uo->uo_record.user.uc0_guid.guid_uid) ) ^ 0x80000000;
         gmask = uo->uo_record.user.uc0_groups;
         groupnames[0] = '\0';
         for (j = 0; j < 32 ; j++ ) {
            test = gmask[j];
            k = 0;
            while (test) {
               if (test & 1) {
                  strcatf(groupnames, mygroup[j * 8 + k].name);
                  strcatf(groupnames, " ");
               }
               test >>= 1;
               k++;
            }
         }

         oprights = uo->uo_record.user.uc0_auth_flags;
         if (oprights & AF_OP_PRINT)
            strcat(groupnames, PRINTOP);
         if (oprights & AF_OP_COMM)
            strcat(groupnames, COMMOP);
         if (oprights & AF_OP_SERVER)
            strcat(groupnames, SERVOP);
         if (oprights & AF_OP_ACCOUNTS)
            strcat(groupnames, ACCTOP);

         printf("\tUser = %ld %Fs %Fs\n", relativeid,
                     (char FAR *) uo->uo_record.name,
                     (char FAR *) groupnames);
         usercount++;
         pos = uo->uo_header.do_next;
      }
      entry++;
   }
   if (usercount != Header->num_users) {
      printf("Huh?  Didn't find all the users (found %d)\n", usercount);
   }
}

void banner(unsigned longnames)
{

   printf(SECPKG);
   printf("\n\n\t//\n\t// Account Setup information\n");
   printf("\t//  This file produced from the LAN Manager 2.0 UAS\n");
   printf("\t//      %s\n", path);

   printf("\tGroup = 501 %s\n", GADMINS);
   printf("\tGroup = 502 %s\n", GUSERS);
   printf("\tGroup = 503 %s\n", GGUESTS);
   printf("\tGroup = 504 %s\n", ACCTOP);
   printf("\tGroup = 505 %s\n", SERVOP);
   printf("\tGroup = 506 %s\n", PRINTOP);
   printf("\tGroup = 507 %s\n", COMMOP);
   printf("\tGroup = 508 %s\n", DISKOP);
   printf("\tGroup = 509 %s\n", AUDITOP);
}

void doargs(int argc, char **argv)
{
   if (argc != 2 || *argv[1] == '?') {
      printf("usage:  %s path\\net.acc\n", argv[0]);
      printf("\tlists all the users/groups and their hash/serial numbers for a UAS\n");
      exit(0);
   }
   strcpy(path, argv[1]);
}

main (int argc, char *argv[])
{
   printf(SIGNON);
   doargs(argc, argv);
   InitMemStuff();
   loadit();

   banner(0);

   printgroups();
   showentries();
}
