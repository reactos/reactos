/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#ifndef FILE_HISTORY_H_
#define FILE_HISTORY_H_

#include "DisplayState.h"
#include "dstring.h"

#define INVALID_MENU_ID (unsigned int)-1

typedef struct FileHistoryList {
    struct FileHistoryList *next;
    unsigned int            menuId;
    DisplayState            state;
} FileHistoryList;

FileHistoryList * FileHistoryList_Node_Create(void);
FileHistoryList * FileHistoryList_Node_CreateFromFilePath(const char *filePath);

void              FileHistoryList_Node_Free(FileHistoryList *node);
void              FileHistoryList_Free(FileHistoryList **root);

void              FileHistoryList_Node_InsertHead(FileHistoryList **root, FileHistoryList *node);
void              FileHistoryList_Node_Append(FileHistoryList **root, FileHistoryList *node);

FileHistoryList * FileHistoryList_Node_FindByFilePath(FileHistoryList **root, const char *filePath);
BOOL              FileHistoryList_Node_RemoveAndFree(FileHistoryList **root, FileHistoryList *node);

BOOL              FileHistoryList_Node_RemoveByFilePath(FileHistoryList **root, const char *filePath);

bool              FileHistoryList_Serialize(FileHistoryList **root, DString *strOut);

#endif
