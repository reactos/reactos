/*
 * FUNCTION: Creates a registry key
 * ARGUMENTS:
 *        KeyHandle (OUT) = Caller supplied storage for the resulting handle
 *        DesiredAccess = Specifies the allowed or desired access to the key
 *          It can have a combination of the following values:
 *          KEY_READ | KEY_WRITE | KEY_EXECUTE | KEY_ALL_ACCESS
 *          or
 *          KEY_QUERY_VALUE The values of the key can be queried.
 *          KEY_SET_VALUE The values of the key can be modified.
 *          KEY_CREATE_SUB_KEYS The key may contain subkeys.
 *          KEY_ENUMERATE_SUB_KEYS Subkeys can be queried.
 *          KEY_NOTIFY
 *          KEY_CREATE_LINK A symbolic link to the key can be created. 
 *        ObjectAttributes = The name of the key may be specified directly in the name field 
 *          of object attributes or relative to a key in rootdirectory.
 *        TitleIndex = Might specify the position in the sequential order of subkeys. 
 *        Class = Specifies the kind of data, for example REG_SZ for string data. [ ??? ]
 *        CreateOptions = Specifies additional options with which the key is created
 *          REG_OPTION_VOLATILE  The key is not preserved across boots.
 *          REG_OPTION_NON_VOLATILE  The key is preserved accross boots.
 *          REG_OPTION_CREATE_LINK  The key is a symbolic link to another key. 
 *          REG_OPTION_BACKUP_RESTORE  Key is being opened or created for backup/restore operations. 
 *        Disposition = Indicates if the call to NtCreateKey resulted in the creation of a key it 
 *          can have the following values: REG_CREATED_NEW_KEY | REG_OPENED_EXISTING_KEY
 * RETURNS:
 *  Status
 */

NTSTATUS STDCALL
NtCreateKey(OUT PHANDLE KeyHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes,
            IN ULONG TitleIndex,
            IN PUNICODE_STRING Class OPTIONAL,
            IN ULONG CreateOptions,
            IN PULONG Disposition OPTIONAL);

NTSTATUS STDCALL
ZwCreateKey(OUT PHANDLE KeyHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes,
            IN ULONG TitleIndex,
            IN PUNICODE_STRING Class OPTIONAL,
            IN ULONG CreateOptions,
            IN PULONG Disposition OPTIONAL);

