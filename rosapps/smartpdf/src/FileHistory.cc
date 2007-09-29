/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#include "FileHistory.h"
#include "str_util.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* Handling of file history list.

   We keep an infinite list of all (still existing in the file system) PDF
   files that a user has ever opened. For each file we also keep a bunch of
   attributes describing the display state at the time the file was closed.

   We persist this list as serialized text inside preferences file.
   Serialized history list looks like this:

File History:
  DisplayMode: single page
  File: /home/test.pdf
  PageNo: 5
  ZoomLevel: 123.3434
  FullScreen: 1
  LastAccessTimeInSecsSinceEpoch: 12341234124314

File History:

etc...

    We deserialize this info at startup and serialize when the application
    quits.
*/

FileHistoryList *FileHistoryList_Node_Create(void)
{
    FileHistoryList *node;
    node = (FileHistoryList*)malloc(sizeof(FileHistoryList));
    if (!node)
        return NULL;

    memzero(node, sizeof(node));
    DisplayState_Init(&(node->state));
    node->menuId = INVALID_MENU_ID;
    return node;
}

FileHistoryList *FileHistoryList_Node_CreateFromFilePath(const char *filePath)
{
    FileHistoryList *node;

    DBG_OUT("FileHistoryList_Node_CreateFromFilePath() file='%s'\n", filePath);
    node = FileHistoryList_Node_Create();
    if (!node)
        return NULL;

    node->state.filePath = (const char*)str_dup(filePath);
    if (!node->state.filePath)
        goto Error;
    return node;

Error:
    FileHistoryList_Node_Free(node);
    return NULL;
}

void FileHistoryList_Node_Free(FileHistoryList *node)
{
    assert(node);
    if (!node) return;
    DisplayState_Free(&(node->state));
    free((void*)node);
}

void FileHistoryList_Free(FileHistoryList **root)
{
    FileHistoryList *curr, *next;
    assert(root);
    if (!root) return;

    curr = *root;
    while (curr) {
        next = curr->next;
        FileHistoryList_Node_Free(curr);
        curr = next;
    }

    *root = NULL;
}

static BOOL FileHistoryList_Node_Serialize(FileHistoryList *node, DString *strOut)
{
    assert(node);
    if (!node) return FALSE;
    assert(strOut);
    if (!strOut) return FALSE;

    DStringSprintf(strOut, "%s:\n", FILE_HISTORY_STR);
    DisplayState_Serialize(&(node->state), strOut);

    return TRUE;
}

bool FileHistoryList_Serialize(FileHistoryList **root, DString *strOut)
{
    assert(root);
    if (!root) return false;

    FileHistoryList *curr = *root;
    while (curr) {
        int fOk = FileHistoryList_Node_Serialize(curr, strOut);
        if (!fOk)
            return false;
        curr = curr->next;
    }
    return true;
}

void FileHistoryList_Node_InsertHead(FileHistoryList **root, FileHistoryList *node)
{
    assert(root);
    if (!root) return;
    node->next = *root;
    *root = node;
}

void FileHistoryList_Node_Append(FileHistoryList **root, FileHistoryList *node)
{
    FileHistoryList *curr;
    assert(root);
    if (!root) return;
    assert(node);
    if (!node) return;
    assert(!node->next);
    curr = *root;
    if (!curr) {
        *root = node;
        return;
    }
    while (curr->next)
        curr = curr->next;
    curr->next = node;
}

FileHistoryList *FileHistoryList_Node_FindByFilePath(FileHistoryList **root, const char *filePath)
{
    FileHistoryList *curr;

    assert(root);
    if (!root) return NULL;
    assert(filePath);
    if (!filePath) return NULL;

    curr = *root;
    while (curr) {
        assert(curr->state.filePath);
        if (str_eq(filePath, curr->state.filePath))
            return curr;
        curr = curr->next;
    }

    return NULL;
}

BOOL FileHistoryList_Node_RemoveByFilePath(FileHistoryList **root, const char *filePath)
{
    FileHistoryList *node;

    assert(root);
    if (!root) return FALSE;
    assert(filePath);
    if (!filePath) return FALSE;

    /* TODO: traversing the list twice, but it's small so we don't care */
    node = FileHistoryList_Node_FindByFilePath(root, filePath);
    if (!node)
        return FALSE;

    return FileHistoryList_Node_RemoveAndFree(root, node);
}

BOOL FileHistoryList_Node_RemoveAndFree(FileHistoryList **root, FileHistoryList *node)
{
    FileHistoryList **prev;

    assert(root);
    if (!root) return FALSE;

    if (node == *root) {
        *root = node->next;
        goto Free;
    }

    prev = root;
    while (*prev) {
        if ((*prev)->next == node) {
            (*prev)->next = node->next;
            goto Free;
        }
        prev = &((*prev)->next);
    }

    /* TODO: should I free it anyway? */
    return FALSE;
Free:
    FileHistoryList_Node_Free(node);
    return TRUE;
}

