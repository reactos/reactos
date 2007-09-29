/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "base_util.h"
#include "file_util.h"

#define DIR_TO_READ "."

int main(int argc, char **argv)
{
    FileList* fileList = FileList_Get(DIR_TO_READ, NULL);
    if (!fileList) {
        printf("Couldn't read dir %s\n", DIR_TO_READ);
        goto Exit;
    }

    FileInfo* fileInfo = fileList->first;
    while (fileInfo) {
        if (FileInfo_IsDir(fileInfo)) {
            printf("d: %s, %s, %d\n", fileInfo->name, fileInfo->path, (int)fileInfo->size);
        } else if (FileInfo_IsFile(fileInfo)) {
            printf("f: %s, %s, %d\n", fileInfo->name, fileInfo->path, (int)fileInfo->size);
        } else {
            printf("Unknown type: %s\n", fileInfo->name);
        }
        fileInfo = fileInfo->next;
    }
Exit:
    FileList_Delete(fileList);
	return 0;
}
