#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>


extern char*_acmdln;
extern char*_pgmptr;
#undef _environ
extern char**_environ;

#undef __argv
#undef __argc

char**__argv = NULL;
int __argc = 0;

extern HANDLE hHeap;

char* strndup(char* name, int len)
{
    char *s = malloc(len + 1);
    if (s != NULL) {
        strncpy(s, name, len);
    }
    return s;
}

#define SIZE (4096 / sizeof(char*))

int add(char* name)
{
    char** _new;
    if ((__argc % SIZE) == 0) {
        _new = malloc(sizeof(char*) * (__argc + SIZE));
        if (_new == NULL) {
            return -1;
        }
        if (__argv) {
            memcpy(_new, __argv, sizeof(char*) * __argc);
            free(__argv);
        }
        __argv = _new;
    }
    __argv[__argc++] = name;
    return 0;
}

int expand(char* name, int flag)
{
    char* s;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    BOOLEAN first = TRUE;
    char buffer[256];
    int pos;

    s = strpbrk(name, "*?");
    if (s && flag) {
        hFile = FindFirstFile(name, &fd);
        if (hFile != INVALID_HANDLE_VALUE) {
            while(s != name && *s != '/' && *s != '\\')
                s--;
            pos = s - name;
            if (*s == '/' || *s == '\\')
                pos++;
            strncpy(buffer, name, pos);
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    strcpy(&buffer[pos], fd.cFileName);
                    if (add(strdup(buffer)) < 0) {
                        FindClose(hFile);
                        return -1;
                    }
                    first = FALSE;
                }
            }
            while(FindNextFile(hFile, &fd));
            FindClose(hFile);
        }
    }
    if (first) {
        if (add(name) < 0)
            return -1;
    }
    else
        free(name);
    return 0;
}

int __getmainargs(int* argc, char*** argv, char*** env, int flag)
{
    int i, afterlastspace, ignorespace, len;

    /* missing threading init */

    i = 0;
    afterlastspace = 0;
    ignorespace = 0;
    len = strlen(_acmdln);

    while (_acmdln[i]) {
	if (_acmdln[i] == '"') {
	    if (_acmdln[i + 1] == '"') {
		memmove(_acmdln + i, _acmdln + i + 1, len - i);
		len--;
	    } else {
		if(ignorespace) {
		    ignorespace = 0;
		} else {
		    ignorespace = 1;
		}
		memmove(_acmdln + i, _acmdln + i + 1, len - i);
		len--;
		continue;
	    }
	}

        if (_acmdln[i] == ' ' && !ignorespace) {
            expand(strndup(_acmdln + afterlastspace, i - afterlastspace), flag);
            i++;
            while (_acmdln[i]==' ')
                i++;
            afterlastspace=i;
        } else {
            i++;
        }
    }

    if (_acmdln[afterlastspace] != 0) {
        expand(strndup(_acmdln+afterlastspace, i - afterlastspace), flag);
    }
    HeapValidate(hHeap, 0, NULL);
    *argc = __argc;
    *argv = __argv;
    *env  = _environ;
    _pgmptr = strdup((char*)argv[0]);
    return 0;
}

int* __p___argc(void)
{
    return &__argc;
}

char*** __p___argv(void)
{
    return &__argv;
}

#if 0
int _chkstk(void)
{
    return 0;
}
#endif
