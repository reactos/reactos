/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              include/internal/objmgr.h
 * PURPOSE:           Object manager definitions
 * PROGRAMMER:        David Welch (welch@mcmail.com)
 */

#ifndef __INCLUDE_INTERNAL_OBJMGR_H
#define __INCLUDE_INTERNAL_OBJMGR_H

#include <ddk/types.h>

typedef struct 
{
   CSHORT Type;
   CSHORT Size;
} COMMON_BODY_HEADER, *PCOMMON_BODY_HEADER;

typedef PVOID POBJECT;

typedef struct _DIRECTORY_OBJECT
{   
   CSHORT Type;
   CSHORT Size;
   
   /*
    * PURPOSE: Head of the list of our subdirectories
    */
   LIST_ENTRY head;
   KSPIN_LOCK Lock;
} DIRECTORY_OBJECT, *PDIRECTORY_OBJECT;


/*
 * Enumeration of object types
 */
enum
{
   OBJTYP_INVALID,
   OBJTYP_TYPE,
   OBJTYP_DIRECTORY,
   OBJTYP_SYMLNK,
   OBJTYP_DEVICE,
   OBJTYP_THREAD,
   OBJTYP_FILE,
   OBJTYP_PROCESS,
   OBJTYP_SECTION,
   OBJTYP_MAX,
};

BOOL ObAddObjectToNameSpace(PUNICODE_STRING path, POBJECT_HEADER Object);

VOID ObRegisterType(CSHORT id, OBJECT_TYPE* type);
NTSTATUS ObLookupObject(HANDLE rootdir, PWSTR string, PVOID* Object,
			 PWSTR* UnparsedSection, ULONG Attributes);
VOID ObRemoveEntry(POBJECT_HEADER Header);
NTSTATUS ObPerformRetentionChecks(POBJECT_HEADER Header);

/*
 * FUNCTION: Creates an entry within a directory
 * ARGUMENTS:
 *        parent = Parent directory
 *        object = Header of the object to add
 */
VOID ObCreateEntry(PDIRECTORY_OBJECT parent, POBJECT_HEADER object);

extern inline POBJECT_HEADER BODY_TO_HEADER(PVOID body)
{
   PCOMMON_BODY_HEADER chdr = (PCOMMON_BODY_HEADER)body;
   return(CONTAINING_RECORD((&(chdr->Type)),OBJECT_HEADER,Type));
}

extern inline PVOID HEADER_TO_BODY(POBJECT_HEADER obj)
{
   return(((void *)obj)+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER));
}

#define OBJECT_ALLOC_SIZE(type) (type->NonpagedPoolCharge+sizeof(OBJECT_HEADER)-sizeof(COMMON_BODY_HEADER))

/*
 * PURPOSE: Defines a handle
 */
typedef struct
{
   PVOID ObjectBody;
   ACCESS_MASK GrantedAccess;
   BOOLEAN Inherit;
} HANDLE_REP, *PHANDLE_REP;

PHANDLE_REP ObTranslateHandle(PKPROCESS Process, HANDLE h);
extern PDIRECTORY_OBJECT NameSpaceRoot;

VOID ObAddEntryDirectory(PDIRECTORY_OBJECT Parent,
			 POBJECT Object,
			 PWSTR Name);
NTSTATUS ObCreateHandle(PEPROCESS Process,
			PVOID ObjectBody,
			ACCESS_MASK GrantedAccess,
			BOOLEAN Inherit,
			PHANDLE Handle);
VOID ObCreateHandleTable(PEPROCESS Parent,
			 BOOLEAN Inherit,
			 PEPROCESS Process);
NTSTATUS ObFindObject(POBJECT_ATTRIBUTES ObjectAttributes,
		      PVOID* ReturnedObject,
		      PUNICODE_STRING RemainingPath);
//		      PWSTR* RemainingPath);

ULONG ObGetReferenceCount(PVOID Object);
ULONG ObGetHandleCount(PVOID Object);
VOID ObCloseAllHandles(PEPROCESS Process);
VOID ObDeleteHandleTable(PEPROCESS Process);

#endif /* __INCLUDE_INTERNAL_OBJMGR_H */
