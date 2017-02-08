/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CD-ROM Maker
 * FILE:            tools/cdmake/dirhash.c
 * PURPOSE:         CD-ROM Premastering Utility - Directory names hashing
 * PROGRAMMERS:     Art Yerkes
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "dirhash.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/* This is the famous DJB hash */
static unsigned int
djb_hash(const char *name)
{
    unsigned int val = 5381;
    int i = 0;

    for (i = 0; name[i]; i++)
    {
        val = (33 * val) + name[i];
    }

    return val;
}

static void
split_path(const char *path, char **dirname, char **filename /* OPTIONAL */)
{
    const char *result;

    /* Retrieve the file name */
    char *last_slash_1 = strrchr(path, '/');
    char *last_slash_2 = strrchr(path, '\\');

    if (last_slash_1 || last_slash_2)
        result = max(last_slash_1, last_slash_2) + 1;
    else
        result = path;

    /* Duplicate the file name for the user if needed */
    if (filename)
        *filename = strdup(result);

    /* Remove any trailing directory separators */
    while (result > path && (*(result-1) == '/' || *(result-1) == '\\'))
        result--;

    /* Retrieve and duplicate the directory */
    *dirname = malloc(result - path + 1);
    if (result > path)
        memcpy(*dirname, path, result - path);
    (*dirname)[result - path] = '\0'; // NULL-terminate
}

void normalize_dirname(char *filename)
{
    int i, tgt;
    int slash = 1;

    for (i = 0, tgt = 0; filename[i]; i++)
    {
        if (slash)
        {
            if (filename[i] != '/' && filename[i] != '\\')
            {
                filename[tgt++] = toupper(filename[i]);
                slash = 0;
            }
        }
        else
        {
            if (filename[i] == '/' || filename[i] == '\\')
            {
                slash = 1;
                filename[tgt++] = DIR_SEPARATOR_CHAR;
            }
            else
            {
                filename[tgt++] = toupper(filename[i]);
            }
        }
    }
    filename[tgt] = '\0'; // NULL-terminate
}

static struct target_dir_entry *
get_entry_by_normname(struct target_dir_hash *dh, const char *norm)
{
    unsigned int hashcode;
    struct target_dir_entry *de;
    hashcode = djb_hash(norm);
    de = dh->buckets[hashcode % NUM_DIR_HASH_BUCKETS];
    while (de && strcmp(de->normalized_name, norm))
        de = de->next_dir_hash_entry;
    return de;
}

static void
delete_entry(struct target_dir_hash *dh, struct target_dir_entry *de)
{
    struct target_dir_entry **ent;
    ent = &dh->buckets[de->hashcode % NUM_DIR_HASH_BUCKETS];
    while (*ent && ((*ent) != de))
        ent = &(*ent)->next_dir_hash_entry;
    if (*ent)
        *ent = (*ent)->next_dir_hash_entry;
}

struct target_dir_entry *
dir_hash_create_dir(struct target_dir_hash *dh, const char *casename, const char *targetnorm)
{
    struct target_dir_entry *de, *parent_de;
    char *parentcase = NULL;
    char *case_name  = NULL;
    char *parentname = NULL;
    struct target_dir_entry **ent;

    if (!dh->root.normalized_name)
    {
        dh->root.normalized_name = strdup("");
        dh->root.case_name = strdup("");
        dh->root.hashcode = djb_hash("");
        dh->buckets[dh->root.hashcode % NUM_DIR_HASH_BUCKETS] = &dh->root;
    }

    /* Check whether the directory was already created and just return it if so */
    de = get_entry_by_normname(dh, targetnorm);
    if (de)
        return de;

    /*
     * If *case_name == '\0' after the following call to split_path(...),
     * for example in the case where casename == "subdir/dir/", then just
     * create the directories "subdir" and "dir" by a recursive call to
     * dir_hash_create_dir(...) and return 'parent_de' instead (see after).
     * We do not (and we never) create a no-name directory inside it.
     */
    split_path(casename, &parentcase, &case_name);
    split_path(targetnorm, &parentname, NULL);
    parent_de = dir_hash_create_dir(dh, parentcase, parentname);
    free(parentname);
    free(parentcase);

    /* See the remark above */
    if (!*case_name)
    {
        free(case_name);
        return parent_de;
    }

    /* Now create the directory */
    de = calloc(1, sizeof(*de));
    de->parent = parent_de;
    de->normalized_name = strdup(targetnorm);
    de->case_name = case_name;
    de->hashcode = djb_hash(targetnorm);

    de->next = parent_de->child;
    parent_de->child = de;

    ent = &dh->buckets[de->hashcode % NUM_DIR_HASH_BUCKETS];
    while (*ent)
        ent = &(*ent)->next_dir_hash_entry;
    *ent = de;

    return de;
}

struct target_file *
dir_hash_add_file(struct target_dir_hash *dh, const char *source, const char *target)
{
    struct target_file *tf;
    struct target_dir_entry *de;
    char *targetdir  = NULL;
    char *targetfile = NULL;
    char *targetnorm;

    /* First create the directory; check whether the file name is valid and bail out if not */
    split_path(target, &targetdir, &targetfile);
    if (!*targetfile)
    {
        free(targetdir);
        free(targetfile);
        return NULL;
    }
    targetnorm = strdup(targetdir);
    normalize_dirname(targetnorm);
    de = dir_hash_create_dir(dh, targetdir, targetnorm);
    free(targetnorm);
    free(targetdir);

    /* Now add the file */
    tf = calloc(1, sizeof(*tf));
    tf->next = de->head;
    de->head = tf;
    tf->source_name = strdup(source);
    tf->target_name = targetfile;

    return tf;
}

static void
dir_hash_destroy_dir(struct target_dir_hash *dh, struct target_dir_entry *de)
{
    struct target_file *tf;
    struct target_dir_entry *te;

    while ((te = de->child))
    {
        de->child = te->next;
        dir_hash_destroy_dir(dh, te);
        free(te);
    }
    while ((tf = de->head))
    {
        de->head = tf->next;
        free(tf->source_name);
        free(tf->target_name);
        free(tf);
    }

    delete_entry(dh, de);
    free(de->normalized_name);
    free(de->case_name);
}

void dir_hash_destroy(struct target_dir_hash *dh)
{
    dir_hash_destroy_dir(dh, &dh->root);
}
