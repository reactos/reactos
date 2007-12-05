#ifndef __INCLUDE_CM_H
#define __INCLUDE_CM_H

#include "ntoskrnl/config/cm.h"

extern POBJECT_TYPE CmpKeyObjectType;
extern ERESOURCE CmpRegistryLock;
extern EX_PUSH_LOCK CmpHiveListHeadLock;

#define VERIFY_BIN_HEADER(x) ASSERT(x->HeaderId == REG_BIN_ID)
#define VERIFY_KEY_CELL(x) ASSERT(x->Signature == CM_KEY_NODE_SIGNATURE)
#define VERIFY_ROOT_KEY_CELL(x) ASSERT(x->Signature == CM_KEY_NODE_SIGNATURE)
#define VERIFY_VALUE_CELL(x) ASSERT(x->Signature == CM_KEY_VALUE_SIGNATURE)
#define VERIFY_VALUE_LIST_CELL(x)
#define VERIFY_KEY_OBJECT(x)
#define VERIFY_REGISTRY_HIVE(x)

#endif /*__INCLUDE_CM_H*/
