#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "dirsep.h"
#include "dirhash.h"

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

static const char *
chop_filename(const char *target)
{
    char *last_slash = strrchr(target, '/');
    if (!last_slash)
        last_slash = strrchr(target, '\\');
    if (last_slash)
        return last_slash + 1;
    else
        return target;
}

static void
chop_dirname(const char *name, char **dirname)
{
    char *last_slash = strrchr(name, '/');
    if (!last_slash)
        last_slash = strrchr(name, '\\');
    if (!last_slash)
    {
        free(*dirname);
        *dirname = malloc(1);
        **dirname = 0;
    }
    else
    {
        char *newdata = malloc(last_slash - name + 1);
        memcpy(newdata, name, last_slash - name);
        newdata[last_slash - name] = 0;
        free(*dirname);
        *dirname = newdata;
    }
}

static struct target_dir_entry *
get_entry_by_normname(struct target_dir_hash *dh, const char *norm)
{
    unsigned int hashcode;
    struct target_dir_entry *de;
    hashcode = djb_hash(norm);
    de = dh->buckets[hashcode % NUM_DIR_HASH_BUCKETS];
    while (de && strcmp(de->normalized_name, norm))
        de = de->next;
    return de;
}

static void
delete_entry_by_normname(struct target_dir_hash *dh, const char *norm)
{
    unsigned int hashcode;
    struct target_dir_entry **ent;
    hashcode = djb_hash(norm);
    ent = &dh->buckets[hashcode % NUM_DIR_HASH_BUCKETS];
    while (*ent && strcmp((*ent)->normalized_name, norm))
        ent = &(*ent)->next;
    if (*ent)
        *ent = (*ent)->next;
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
    filename[tgt] = 0;

    while (tgt && (filename[--tgt] == DIR_SEPARATOR_CHAR))
    {
        filename[tgt] = 0;
    }
}

struct target_dir_entry *
dir_hash_create_dir(struct target_dir_hash *dh, const char *casename, const char *targetnorm)
{
    unsigned int hashcode;
    struct target_dir_entry *de, *parent_de;
    char *parentname = NULL;
    char *parentcase = NULL;
    struct target_dir_entry **ent;
    if (!dh->root.normalized_name)
    {
        dh->root.normalized_name = strdup("");
        dh->root.case_name = strdup("");
        hashcode = djb_hash("");
        dh->buckets[hashcode % NUM_DIR_HASH_BUCKETS] = &dh->root;
    }
    de = get_entry_by_normname(dh, targetnorm);
    if (de)
        return de;
    chop_dirname(targetnorm, &parentname);
    chop_dirname(casename, &parentcase);
    parent_de = dir_hash_create_dir(dh, parentcase, parentname);
    free(parentname);
    free(parentcase);
    hashcode = djb_hash(targetnorm);
    de = calloc(1, sizeof(*de));
    de->parent = parent_de;
    de->normalized_name = strdup(targetnorm);
    de->case_name = strdup(chop_filename(casename));
    de->next = parent_de->child;
    parent_de->child = de;
    ent = &dh->buckets[hashcode % NUM_DIR_HASH_BUCKETS];
    while ((*ent))
    {
        ent = &(*ent)->next;
    }
    *ent = de;
    return de;
}

void dir_hash_add_file(struct target_dir_hash *dh, const char *source, const char *target)
{
    struct target_file *tf;
    struct target_dir_entry *de;
    const char *filename = chop_filename(target);
    char *targetdir = NULL;
    char *targetnorm;
    chop_dirname(target, &targetdir);
    targetnorm = strdup(targetdir);
    normalize_dirname(targetnorm);
    de = dir_hash_create_dir(dh, targetdir, targetnorm);
    free(targetnorm);
    free(targetdir);
    tf = calloc(1, sizeof(*tf));
    tf->next = de->head;
    de->head = tf;
    tf->source_name = strdup(source);
    tf->target_name = strdup(filename);
}

static struct target_dir_entry *
dir_hash_next_dir(struct target_dir_hash *dh, struct target_dir_traversal *t)
{
    if (t->i == -1)
        return NULL;
    if (!t->it)
    {
        while (++t->i != NUM_DIR_HASH_BUCKETS)
        {
            if (dh->buckets[t->i])
            {
                t->it = dh->buckets[t->i];
                return t->it;
            }
        }
        t->i = -1;
        return NULL;
    }
    else
    {
        t->it = t->it->next;
        if (!t->it)
        {
            t->i = -1;
            return NULL;
        }
        else
            return t->it;
    }
}

static void
dir_hash_destroy_dir(struct target_dir_hash *dh, struct target_dir_entry *de)
{
    struct target_file *tf;
    struct target_dir_entry *te;
    unsigned int hashcode;
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
    if (de->normalized_name)
        delete_entry_by_normname(dh, de->normalized_name);
    free(de->normalized_name);
    free(de->case_name);
}

void dir_hash_destroy(struct target_dir_hash *dh)
{
    dir_hash_destroy_dir(dh, &dh->root);
}
