#ifndef NDEBUG
#define TEST_STATUS(s) \
        if (! NT_SUCCESS(s)) \
        { \
            if (s == STATUS_NO_MORE_ENTRIES) \
                DPRINT("NTSTATUS == NO MORE ENTRIES\n") \
            else if (s == STATUS_BUFFER_OVERFLOW) \
                DPRINT("NTSTATUS == BUFFER OVERFLOW\n") \
            else if (s == STATUS_BUFFER_TOO_SMALL) \
                DPRINT("NTSTATUS == BUFFER TOO SMALL\n") \
            else if (s == STATUS_INVALID_PARAMETER) \
                DPRINT("NTSTATUS == INVALID PARAMETER\n") \
            else if (s == STATUS_OBJECT_NAME_NOT_FOUND) \
                DPRINT("NTSTATUS == OBJECT NAME NOT FOUND\n") \
            else if (s == STATUS_INVALID_HANDLE) \
                DPRINT("NTATATUS == INVALID_HANDLE\n") \
            else if (s == STATUS_ACCESS_DENIED) \
                DPRINT("NTSTATUS == ACCESS_DENIED\n") \
            else \
                DPRINT("NTSTATUS == FAILURE (???)\n"); \
        }
#else
#define TEST_STATUS(s)
#endif
