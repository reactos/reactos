/*++ BUILD Version: 0002    // Increment this if a change has global effects
---*/

extern EI Ei;
#define Lpei (&Ei)

#define EEFreeStr           (Lpei)->pStructExprAPI->pEEFreeStr
#define EEGetError          (Lpei)->pStructExprAPI->pEEGetError
#define EEParse             (Lpei)->pStructExprAPI->pEEParse
#define EEBindTM            (Lpei)->pStructExprAPI->pEEBindTM
#define EEvaluateTM         (Lpei)->pStructExprAPI->pEEvaluateTM
#define EEGetExprFromTM     (Lpei)->pStructExprAPI->pEEGetExprFromTM
#define EEGetValueFromTM    (Lpei)->pStructExprAPI->pEEGetValueFromTM
#define EEGetNameFromTM     (Lpei)->pStructExprAPI->pEEGetNameFromTM
#define EEGetTypeFromTM     (Lpei)->pStructExprAPI->pEEGetTypeFromTM
#define EEFormatCXTFromPCXT (Lpei)->pStructExprAPI->pEEFormatCXTFromPCXT
#define EEFreeTM            (Lpei)->pStructExprAPI->pEEFreeTM
#define EEParseBP           (Lpei)->pStructExprAPI->pEEParseBP
#define EEFreeTML           (Lpei)->pStructExprAPI->pEEFreeTML
#define EEInfoFromTM        (Lpei)->pStructExprAPI->pEEInfoFromTM
#define EEFreeTI            (Lpei)->pStructExprAPI->pEEFreeTI
#define EEGetCXTLFromTM     (Lpei)->pStructExprAPI->pEEGetCXTLFromTM
#define EEFreeCXTL          (Lpei)->pStructExprAPI->pEEFreeCXTL
#define EEAssignTMToTM      (Lpei)->pStructExprAPI->pEEAssignTMToTM
#define EEIsExpandable      (Lpei)->pStructExprAPI->pEEIsExpandable
#define EEAreTypesEqual     (Lpei)->pStructExprAPI->pEEAreTypesEqual
#define EEGetHtypeFromTM    (Lpei)->pStructExprAPI->pEEGetHtypeFromTM
#define EEcChildrenTM       (Lpei)->pStructExprAPI->pEEcChildrenTM
#define EEGetChildTM        (Lpei)->pStructExprAPI->pEEGetChildTM
#define EEDereferenceTM     (Lpei)->pStructExprAPI->pEEDereferenceTM
#define EEcParamTM          (Lpei)->pStructExprAPI->pEEcParamTM
#define EEGetParmTM         (Lpei)->pStructExprAPI->pEEGetParmTM
#define EEGetTMFromHSYM     (Lpei)->pStructExprAPI->pEEGetTMFromHSYM
#define EEFormatAddress     (Lpei)->pStructExprAPI->pEEFormatAddress
#define EEGetHSYMList       (Lpei)->pStructExprAPI->pEEGetHSYMList
#define EEFreeHSYMList      (Lpei)->pStructExprAPI->pEEFreeHSYMList
#define EEFormatAddress     (Lpei)->pStructExprAPI->pEEFormatAddress
#define EEUnFormatAddr      (Lpei)->pStructExprAPI->pEEUnFormatAddr
#define EEFormatEnumerate   (Lpei)->pStructExprAPI->pEEFormatEnumerate
#define EEFormatMemory      (Lpei)->pStructExprAPI->pEEFormatMemory
#define EEUnformatMemory    (Lpei)->pStructExprAPI->pEEUnformatMemory
#define EESetSuffix         (Lpei)->pStructExprAPI->pEESetSuffix
#define EEInvalidateCache   (Lpei)->pStructExprAPI->pEEInvalidateCache
#define EESetTarget         (Lpei)->pStructExprAPI->pEESetTarget
#define EEUnload            (Lpei)->pStructExprAPI->pEEUnload
