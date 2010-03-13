$if (_WDMDDK_)
#ifndef _NTTMAPI_
#define _NTTMAPI_

#include <ktmtypes.h>

typedef enum _KTMOBJECT_TYPE {
  KTMOBJECT_TRANSACTION,
  KTMOBJECT_TRANSACTION_MANAGER,
  KTMOBJECT_RESOURCE_MANAGER,
  KTMOBJECT_ENLISTMENT,
  KTMOBJECT_INVALID
} KTMOBJECT_TYPE, *PKTMOBJECT_TYPE;

typedef struct _KTMOBJECT_CURSOR {
  GUID LastQuery;
  ULONG ObjectIdCount;
  GUID ObjectIds[1];
} KTMOBJECT_CURSOR, *PKTMOBJECT_CURSOR;

typedef enum _TRANSACTION_INFORMATION_CLASS {
  TransactionBasicInformation,
  TransactionPropertiesInformation,
  TransactionEnlistmentInformation,
  TransactionSuperiorEnlistmentInformation
} TRANSACTION_INFORMATION_CLASS;

typedef enum _TRANSACTIONMANAGER_INFORMATION_CLASS {
  TransactionManagerBasicInformation,
  TransactionManagerLogInformation,
  TransactionManagerLogPathInformation,
  TransactionManagerRecoveryInformation = 4
} TRANSACTIONMANAGER_INFORMATION_CLASS;

typedef enum _RESOURCEMANAGER_INFORMATION_CLASS {
  ResourceManagerBasicInformation,
  ResourceManagerCompletionInformation,
} RESOURCEMANAGER_INFORMATION_CLASS;

typedef enum _ENLISTMENT_INFORMATION_CLASS {
  EnlistmentBasicInformation,
  EnlistmentRecoveryInformation,
  EnlistmentCrmInformation
} ENLISTMENT_INFORMATION_CLASS;

#endif /* !_NTTMAPI_ */
$endif
