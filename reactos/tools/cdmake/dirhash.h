/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CD-ROM Maker
 * FILE:            tools/cdmake/dirhash.h
 * PURPOSE:         CD-ROM Premastering Utility - Directory names hashing
 * PROGRAMMERS:     Art Yerkes
 */
#ifndef _DIRHASH_H_
#define _DIRHASH_H_

#define NUM_DIR_HASH_BUCKETS 1024

struct target_file
{
    struct target_file *next;
    char *source_name;
    char *target_name;
};

struct target_dir_entry
{
    unsigned int hashcode;
    struct target_dir_entry *next_dir_hash_entry;

    struct target_dir_entry *next;
    struct target_dir_entry *parent;
    struct target_dir_entry *child;
    struct target_file *head;
    char *normalized_name;
    char *case_name;
};

struct target_dir_hash
{
    struct target_dir_entry *buckets[NUM_DIR_HASH_BUCKETS];
    struct target_dir_entry root;
};

void normalize_dirname(char *filename);
void dir_hash_add_file(struct target_dir_hash *dh, const char *source, const char *target);
struct target_dir_entry *
dir_hash_create_dir(struct target_dir_hash *dh, const char *casename, const char *targetnorm);
void dir_hash_destroy(struct target_dir_hash *dh);

#endif // _DIRHASH_H_
