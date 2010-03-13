/******************************************************************************
 *                            Object Manager Types                            *
 ******************************************************************************/

typedef struct _OBJECT_NAME_INFORMATION {
  UNICODE_STRING  Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

/* Exported object types */
extern POBJECT_TYPE NTSYSAPI ExEventObjectType;
extern POBJECT_TYPE NTSYSAPI ExSemaphoreObjectType;
extern POBJECT_TYPE NTSYSAPI IoFileObjectType;
extern POBJECT_TYPE NTSYSAPI PsThreadType;
extern POBJECT_TYPE NTSYSAPI SeTokenObjectType;
extern POBJECT_TYPE NTSYSAPI PsProcessType;


