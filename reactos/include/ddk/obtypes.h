struct _DIRECTORY_OBJECT;

typedef ULONG ACCESS_STATE, *PACCESS_STATE;

typedef struct _OBJECT_HANDLE_INFORMATION {
    ULONG HandleAttributes;
    ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;

struct _OBJECT;


typedef struct _OBJECT_TYPE
{   
   /*
    * PURPOSE: Name of the type
    */
   UNICODE_STRING TypeName;
   
   /*
    * PURPOSE: Total number of objects of this type
    */
   ULONG TotalObjects;
   
   /*
    * PURPOSE: Total number of handles of this type
    */
   ULONG TotalHandles;
   
   /*
    * PURPOSE: Maximum objects of this type
    */
   ULONG MaxObjects;
   
   /*
    * PURPOSE: Maximum handles of this type
    */
   ULONG MaxHandles;
   
   /*
    * PURPOSE: Paged pool charge
    */
   ULONG PagedPoolCharge;
   
   /*
    * PURPOSE: Nonpaged pool charge
    */
   ULONG NonpagedPoolCharge;
   
   /*
    * PURPOSE: Dumps the object
    * NOTE: To be defined
    */
   VOID (*Dump)(VOID);
   
   /*
    * PURPOSE: Opens the object
    * NOTE: To be defined
    */
   VOID (*Open)(VOID);
   
   /*
    * PURPOSE: Called to close an object if OkayToClose returns true
    */
   VOID (*Close)(PVOID ObjectBody);

   /*
    * PURPOSE: Called to close an object if OkayToClose returns true
    */
   VOID (*Delete)(VOID);

   /*
    * PURPOSE: Called when an open attempts to open a file apparently
    * residing within the object
    */
   PVOID (*Parse)(struct _OBJECT *ParsedObject, PWSTR UnparsedSection);
   
   /*
    */
   VOID (*Security)(VOID);
   
   /*
    */
   VOID (*QueryName)(VOID);
   
   /*
    * PURPOSE: Called when a process asks to close the object
    */
   VOID (*OkayToClose)(VOID);
         
} OBJECT_TYPE, *POBJECT_TYPE;


typedef struct _OBJECT_HEADER
/*
 * PURPOSE: Header for every object managed by the object manager
 */
{   
   UNICODE_STRING Name;
   LIST_ENTRY Entry;
   LONG RefCount;   
   LONG HandleCount;   
   BOOLEAN Permanent;   
   struct _DIRECTORY_OBJECT* Parent;   
   POBJECT_TYPE ObjectType;
   
   /*
    * PURPOSE: Object type
    * NOTE: This overlaps the first member of the object body
    */
   CSHORT Type;
   
   /*
    * PURPOSE: Object size
    * NOTE: This overlaps the second member of the object body
    */
   CSHORT Size;
   
   
} OBJECT_HEADER, *POBJECT_HEADER;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	SECURITY_DESCRIPTOR *SecurityDescriptor;       
	SECURITY_QUALITY_OF_SERVICE *SecurityQualityOfService;  
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _HANDLE_TABLE
{
   LIST_ENTRY ListHead;
   KSPIN_LOCK ListLock;
} HANDLE_TABLE, *PHANDLE_TABLE;
