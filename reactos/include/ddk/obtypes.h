struct _DIRECTORY_OBJECT;

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
   VOID (*Close)(VOID);

   /*
    * PURPOSE: Called to close an object if OkayToClose returns true
    */
   VOID (*Delete)(VOID);

   /*
    * PURPOSE: Called when an open attempts to open a file apparently
    * residing within the object
    */
   VOID (*Parse)(VOID);
   
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


typedef struct _OBJECT
/*
 * PURPOSE: Header for every object managed by the object manager
 */
{   
   /*
    * PURPOSE: Name of this entry
    */
   UNICODE_STRING name;
   
   /*
    * PURPOSE: Our entry in our parents list of subdirectory
    */
   LIST_ENTRY entry;

   /*
    * PURPOSE: Number of non-handle references to this object
    */
   ULONG RefCount;
   
   /*
    * PURPOSE: Number of handles opened to this object
    */
   ULONG HandleCount;
   
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

/*
 * PURPOSE: Defines an object 
 */
typedef struct _OBJECT_ATTRIBUTES
{
   ULONG Length;
   HANDLE RootDirectory;
   PUNICODE_STRING ObjectName;
   ULONG Attributes;
   PVOID SecurityDescriptor;
   PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
