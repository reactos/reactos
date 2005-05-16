#ifndef _INCLUDE_DDK_OBFUNCS_H
#define _INCLUDE_DDK_OBFUNCS_H
/* OBJECT MANAGER ************************************************************/

typedef enum _OB_OPEN_REASON
{    
    ObCreateHandle,
    ObOpenHandle,
    ObDuplicateHandle,
    ObInheritHandle,
    ObMaxOpenReason
} OB_OPEN_REASON;
    
/* TEMPORARY HACK */
typedef NTSTATUS STDCALL_FUNC
(*OB_CREATE_METHOD)(PVOID ObjectBody,
                     PVOID Parent,
                     PWSTR RemainingPath,
                     struct _OBJECT_ATTRIBUTES* ObjectAttributes);
                         
/* Object Callbacks */
typedef NTSTATUS STDCALL_FUNC
(*OB_OPEN_METHOD)(OB_OPEN_REASON Reason,
                  PVOID ObjectBody,
                  PEPROCESS Process,
                  ULONG HandleCount,
                  ACCESS_MASK GrantedAccess);

typedef NTSTATUS STDCALL_FUNC
(*OB_PARSE_METHOD)(PVOID Object,
                    PVOID *NextObject,
                    PUNICODE_STRING FullPath,
                    PWSTR *Path,
                    ULONG Attributes);
                        
typedef VOID STDCALL_FUNC
(*OB_DELETE_METHOD)(PVOID DeletedObject);

typedef VOID STDCALL_FUNC
(*OB_CLOSE_METHOD)(PVOID ClosedObject, ULONG HandleCount);

typedef VOID STDCALL_FUNC
(*OB_DUMP_METHOD)(VOID);

typedef NTSTATUS STDCALL_FUNC
(*OB_OKAYTOCLOSE_METHOD)(VOID);

typedef NTSTATUS STDCALL_FUNC
(*OB_QUERYNAME_METHOD)(PVOID ObjectBody,
                        POBJECT_NAME_INFORMATION ObjectNameInfo,
                        ULONG Length,
                        PULONG ReturnLength);

typedef PVOID STDCALL_FUNC
(*OB_FIND_METHOD)(PVOID WinStaObject,
                   PWSTR Name,
                   ULONG Attributes);

typedef NTSTATUS STDCALL_FUNC
(*OB_SECURITY_METHOD)(PVOID ObjectBody,
                        SECURITY_OPERATION_CODE OperationCode,
                        SECURITY_INFORMATION SecurityInformation,
                        PSECURITY_DESCRIPTOR SecurityDescriptor,
                        PULONG BufferLength);

typedef struct _OBJECT_CREATE_INFORMATION 
{
    ULONG Attributes;
    HANDLE RootDirectory;
    PVOID ParseContext;
    KPROCESSOR_MODE ProbeMode;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    ULONG SecurityDescriptorCharge;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQos;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

typedef struct _OBJECT_TYPE_INITIALIZER
{
    WORD Length;
    UCHAR UseDefaultObject;
    UCHAR CaseInsensitive;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    UCHAR SecurityRequired;
    UCHAR MaintainHandleCount;
    UCHAR MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
    OB_DUMP_METHOD DumpProcedure;
    OB_OPEN_METHOD OpenProcedure;
    OB_CLOSE_METHOD CloseProcedure;
    OB_DELETE_METHOD DeleteProcedure;
    OB_PARSE_METHOD ParseProcedure;
    OB_SECURITY_METHOD SecurityProcedure;
    OB_QUERYNAME_METHOD QueryNameProcedure;
    OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

typedef struct _OBJECT_TYPE
{
    ERESOURCE Mutex;                    /* Used to lock the Object Type */
    LIST_ENTRY TypeList;                /* Links all the Types Together for Debugging */
    UNICODE_STRING Name;                /* Name of the Type */
    PVOID DefaultObject;                /* What Object to use during a Wait (ie, FileObjects wait on FileObject->Event) */
    ULONG Index;                        /* Index of this Type in the Object Directory */
    ULONG TotalNumberOfObjects;         /* Total number of objects of this type */
    ULONG TotalNumberOfHandles;         /* Total number of handles of this type */
    ULONG HighWaterNumberOfObjects;     /* Peak number of objects of this type */
    ULONG HighWaterNumberOfHandles;     /* Peak number of handles of this type */
    OBJECT_TYPE_INITIALIZER TypeInfo;   /* Information captured during type creation */
    ULONG Key;                          /* Key to use when allocating objects of this type */
    ERESOURCE ObjectLocks[4];           /* Locks for locking the Objects */
} OBJECT_TYPE;

NTSTATUS STDCALL
ObAssignSecurity(IN PACCESS_STATE AccessState,
		 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		 IN PVOID Object,
		 IN POBJECT_TYPE Type);

NTSTATUS STDCALL
ObCreateObject (IN KPROCESSOR_MODE ObjectAttributesAccessMode OPTIONAL,
		IN POBJECT_TYPE ObjectType,
		IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
		IN KPROCESSOR_MODE AccessMode,
		IN OUT PVOID ParseContext OPTIONAL,
		IN ULONG ObjectSize,
		IN ULONG PagedPoolCharge OPTIONAL,
		IN ULONG NonPagedPoolCharge OPTIONAL,
		OUT PVOID *Object);

VOID FASTCALL
ObfDereferenceObject(IN PVOID Object);

VOID FASTCALL
ObfReferenceObject(IN PVOID Object);

#define ObDereferenceObject(Object) \
  ObfDereferenceObject(Object)

#define ObReferenceObject(Object) \
  ObfReferenceObject(Object)

ULONG STDCALL
ObGetObjectPointerCount(IN PVOID Object);

NTSTATUS STDCALL
ObGetObjectSecurity(IN PVOID Object,
		    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
		    OUT PBOOLEAN MemoryAllocated);

NTSTATUS STDCALL
ObInsertObject(IN PVOID Object,
	       IN PACCESS_STATE PassedAccessState OPTIONAL,
	       IN ACCESS_MASK DesiredAccess,
	       IN ULONG AdditionalReferences,
	       OUT PVOID* ReferencedObject OPTIONAL,
	       OUT PHANDLE Handle);

VOID STDCALL
ObMakeTemporaryObject(IN PVOID ObjectBody);

NTSTATUS STDCALL
ObOpenObjectByName(IN POBJECT_ATTRIBUTES ObjectAttributes,
		   IN POBJECT_TYPE ObjectType,
		   IN OUT PVOID ParseContext OPTIONAL,
		   IN KPROCESSOR_MODE AccessMode,
		   IN ACCESS_MASK DesiredAccess,
		   IN PACCESS_STATE PassedAccessState,
		   OUT PHANDLE Handle);

NTSTATUS STDCALL
ObOpenObjectByPointer(IN PVOID Object,
		      IN ULONG HandleAttributes,
		      IN PACCESS_STATE PassedAccessState OPTIONAL,
		      IN ACCESS_MASK DesiredAccess OPTIONAL,
		      IN POBJECT_TYPE ObjectType OPTIONAL,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PHANDLE Handle);

NTSTATUS STDCALL
ObQueryNameString(IN PVOID Object,
		  OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
		  IN ULONG Length,
		  OUT PULONG ReturnLength);

NTSTATUS STDCALL
ObQueryObjectAuditingByHandle(IN HANDLE Handle,
			      OUT PBOOLEAN GenerateOnClose);

/*
 * FUNCTION: Performs access validation on an object handle and if access
 * is granted returns a pointer to the object's body
 * ARGUMENTS:
 *         Handle = Handle to the object 
 *         DesiredAccess = Desired access to the object
 *         ObjectType (OPTIONAL) = Pointer to the object's type definition
 *         AccessMode = Access mode to use for the check
 *         Object (OUT) = Caller supplied storage for a pointer to the object's
 *                        body
 *         HandleInformation (OUT) = Points to a structure which receives
 *                                   information about the handle
 * RETURNS: Status
 */
NTSTATUS STDCALL
ObReferenceObjectByHandle(IN HANDLE Handle,
			  IN ACCESS_MASK DesiredAccess,
			  IN POBJECT_TYPE ObjectType OPTIONAL,
			  IN KPROCESSOR_MODE AccessMode,
			  OUT PVOID* Object,
			  OUT POBJECT_HANDLE_INFORMATION HandleInfo OPTIONAL);

/*
 * FUNCTION: Increments the reference count for a given object
 * ARGUMENTS:
 *      Object = Points to the body of the object
 *      AccessMode = Requested access to the object
 *      ObjectType = Pointer to the object's type definition
 *      AccessMode = Access mode to use for the security check
 * RETURNS: Status
 */
NTSTATUS STDCALL
ObReferenceObjectByPointer(IN PVOID Object,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_TYPE ObjectType,
			   IN KPROCESSOR_MODE AccessMode);

NTSTATUS STDCALL
ObReferenceObjectByName(IN PUNICODE_STRING ObjectPath,
			IN ULONG Attributes,
			IN PACCESS_STATE PassedAccessState OPTIONAL,
			IN ACCESS_MASK DesiredAccess OPTIONAL,
			IN POBJECT_TYPE ObjectType,
			IN KPROCESSOR_MODE AccessMode,
			IN OUT PVOID ParseContext OPTIONAL,
			OUT PVOID* ObjectPtr);

VOID STDCALL
ObReleaseObjectSecurity(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			IN BOOLEAN MemoryAllocated);

/*
NTSTATUS STDCALL
ObSetSecurityDescriptorInfo(IN PVOID Object,
			    IN PSECURITY_INFORMATION SecurityInformation,
			    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			    ULONG Param4,
			    IN POOL_TYPE PoolType,
			    IN PGENERIC_MAPPING GenericMapping);
*/

NTSTATUS STDCALL
ObFindHandleForObject(IN PEPROCESS Process,
                      IN PVOID Object,
                      IN POBJECT_TYPE ObjectType,
                      IN POBJECT_HANDLE_INFORMATION HandleInformation,
                      OUT PHANDLE HandleReturn);

#endif /* ndef _INCLUDE_DDK_OBFUNCS_H */
