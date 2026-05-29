  3 stub GetProxyDllInfo

400 stub PROPSYS_400
402 stub PROPSYS_402
403 stub PROPSYS_403
404 stub PROPSYS_404
405 stub PROPSYS_405
406 stub PROPSYS_406
407 stub PROPSYS_407
408 stub PROPSYS_408
409 stub PROPSYS_409
410 stub PROPSYS_410
411 stub PROPSYS_411
412 stub PROPSYS_412
413 stub PROPSYS_413
414 stub PROPSYS_414
415 stub PROPSYS_415
416 stub PROPSYS_416
417 stub PROPSYS_417
418 stub PROPSYS_418
420 stub PROPSYS_420
421 stub PROPSYS_421
422 stub PROPSYS_422

@ stdcall ClearPropVariantArray(ptr long)
@ stdcall ClearVariantArray(ptr long)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall InitPropVariantFromBooleanVector(ptr long ptr)
@ stdcall InitPropVariantFromBuffer(ptr long ptr)
@ stdcall InitPropVariantFromCLSID(ptr ptr)
@ stdcall InitPropVariantFromDoubleVector(ptr long ptr)
@ stdcall InitPropVariantFromFileTime(ptr ptr)
@ stub InitPropVariantFromFileTimeVector
@ stdcall InitPropVariantFromGUIDAsString(ptr ptr)
@ stdcall InitPropVariantFromInt16Vector(ptr long ptr)
@ stdcall InitPropVariantFromInt32Vector(ptr long ptr)
@ stdcall InitPropVariantFromInt64Vector(ptr long ptr)
@ stdcall InitPropVariantFromPropVariantVectorElem(ptr long ptr)
@ stdcall InitPropVariantFromResource(ptr long ptr)
@ stdcall InitPropVariantFromStrRet(ptr ptr ptr)
@ stub InitPropVariantFromStringAsVector
@ stdcall InitPropVariantFromStringVector(ptr long ptr)
@ stdcall InitPropVariantFromUInt16Vector(ptr long ptr)
@ stdcall InitPropVariantFromUInt32Vector(ptr long ptr)
@ stdcall InitPropVariantFromUInt64Vector(ptr long ptr)
@ stdcall InitPropVariantVectorFromPropVariant(ptr ptr)
@ stub InitVariantFromBooleanArray
@ stdcall InitVariantFromBuffer(ptr long ptr)
@ stub InitVariantFromDoubleArray
@ stdcall InitVariantFromFileTime(ptr ptr)
@ stub InitVariantFromFileTimeArray
@ stdcall InitVariantFromGUIDAsString(ptr ptr)
@ stub InitVariantFromInt16Array
@ stub InitVariantFromInt32Array
@ stub InitVariantFromInt64Array
@ stub InitVariantFromResource
@ stdcall InitVariantFromStrRet(ptr ptr ptr)
@ stub InitVariantFromStringArray
@ stub InitVariantFromUInt16Array
@ stub InitVariantFromUInt32Array
@ stub InitVariantFromUInt64Array
@ stub InitVariantFromVariantArrayElem
@ stub PSCoerceToCanonicalValue
@ stub PSCreateAdapterFromPropertyStore
@ stub PSCreateDelayedMultiplexPropertyStore
@ stdcall PSCreateMemoryPropertyStore(ptr ptr)
@ stub PSCreateMultiplexPropertyStore
@ stub PSCreatePropertyChangeArray
@ stdcall PSCreatePropertyStoreFromObject(ptr long ptr ptr)
@ stub PSCreatePropertyStoreFromPropertySetStorage
@ stub PSCreateSimplePropertyChange
@ stub PSEnumeratePropertyDescriptions
@ stdcall PSFormatForDisplay(ptr ptr long ptr long)
@ stdcall PSFormatForDisplayAlloc(ptr ptr long ptr)
@ stub PSFormatPropertyValue
@ stub PSGetItemPropertyHandler
@ stub PSGetItemPropertyHandlerWithCreateObject
@ stub PSGetNameFromPropertyKey
@ stub PSGetNamedPropertyFromPropertyStorage
@ stdcall PSGetPropertyDescription(ptr ptr ptr)
@ stub PSGetPropertyDescriptionByName
@ stdcall PSGetPropertyDescriptionListFromString(wstr ptr ptr)
@ stub PSGetPropertyFromPropertyStorage
@ stdcall PSGetPropertyKeyFromName(wstr ptr)
@ stdcall PSGetPropertySystem(ptr ptr)
@ stub PSGetPropertyValue
@ stub PSLookupPropertyHandlerCLSID
@ stdcall PSPropertyKeyFromString(wstr ptr)
@ stdcall PSRefreshPropertySchema()
@ stdcall PSRegisterPropertySchema(wstr)
@ stub PSSetPropertyValue
@ stdcall PSStringFromPropertyKey(ptr ptr long)
@ stdcall PSUnregisterPropertySchema(wstr)
@ stdcall PropVariantChangeType(ptr ptr long long)
@ stdcall PropVariantCompareEx(ptr ptr long long)
@ stdcall PropVariantGetBooleanElem(ptr long ptr)
@ stdcall PropVariantGetDoubleElem(ptr long ptr)
@ stdcall PropVariantGetElementCount(ptr)
@ stdcall PropVariantGetFileTimeElem(ptr long ptr)
@ stdcall PropVariantGetInt16Elem(ptr long ptr)
@ stdcall PropVariantGetInt32Elem(ptr long ptr)
@ stdcall PropVariantGetInt64Elem(ptr long ptr)
@ stdcall PropVariantGetStringElem(ptr long ptr)
@ stdcall PropVariantGetUInt16Elem(ptr long ptr)
@ stdcall PropVariantGetUInt32Elem(ptr long ptr)
@ stdcall PropVariantGetUInt64Elem(ptr long ptr)
@ stdcall PropVariantToBSTR(ptr ptr)
@ stdcall PropVariantToBoolean(ptr ptr)
@ stub PropVariantToBooleanVector
@ stub PropVariantToBooleanVectorAlloc
@ stdcall PropVariantToBooleanWithDefault(ptr long)
@ stdcall PropVariantToBuffer(ptr ptr long)
@ stdcall PropVariantToDouble(ptr ptr)
@ stub PropVariantToDoubleVector
@ stub PropVariantToDoubleVectorAlloc
@ stdcall PropVariantToDoubleWithDefault(ptr double)
@ stdcall PropVariantToFileTime(ptr long ptr)
@ stub PropVariantToFileTimeVector
@ stub PropVariantToFileTimeVectorAlloc
@ stdcall PropVariantToGUID(ptr ptr)
@ stdcall PropVariantToInt16(ptr ptr)
@ stub PropVariantToInt16Vector
@ stub PropVariantToInt16VectorAlloc
@ stub PropVariantToInt16WithDefault
@ stdcall PropVariantToInt32(ptr ptr)
@ stub PropVariantToInt32Vector
@ stub PropVariantToInt32VectorAlloc
@ stdcall PropVariantToInt32WithDefault(ptr long)
@ stdcall PropVariantToInt64(ptr ptr)
@ stub PropVariantToInt64Vector
@ stub PropVariantToInt64VectorAlloc
@ stdcall PropVariantToInt64WithDefault(ptr int64)
@ stdcall PropVariantToStrRet(ptr ptr)
@ stdcall PropVariantToString(ptr ptr long)
@ stdcall PropVariantToStringAlloc(ptr ptr)
@ stdcall PropVariantToStringVector(ptr ptr long)
@ stdcall PropVariantToStringVectorAlloc(ptr ptr ptr)
@ stdcall PropVariantToStringWithDefault(ptr wstr)
@ stdcall PropVariantToUInt16(ptr ptr)
@ stub PropVariantToUInt16Vector
@ stub PropVariantToUInt16VectorAlloc
@ stdcall PropVariantToUInt16WithDefault(ptr long)
@ stdcall PropVariantToUInt32(ptr ptr)
@ stub PropVariantToUInt32Vector
@ stub PropVariantToUInt32VectorAlloc
@ stdcall PropVariantToUInt32WithDefault(ptr long)
@ stdcall PropVariantToUInt64(ptr ptr)
@ stub PropVariantToUInt64Vector
@ stub PropVariantToUInt64VectorAlloc
@ stdcall PropVariantToUInt64WithDefault(ptr int64)
@ stdcall PropVariantToVariant(ptr ptr)
@ stub StgDeserializePropVariant
@ stub StgSerializePropVariant
@ stub VariantCompare
@ stub VariantGetBooleanElem
@ stub VariantGetDoubleElem
@ stdcall VariantGetElementCount(ptr)
@ stub VariantGetInt16Elem
@ stub VariantGetInt32Elem
@ stub VariantGetInt64Elem
@ stub VariantGetStringElem
@ stub VariantGetUInt16Elem
@ stub VariantGetUInt32Elem
@ stub VariantGetUInt64Elem
@ stdcall VariantToBoolean(ptr ptr)
@ stub VariantToBooleanArray
@ stub VariantToBooleanArrayAlloc
@ stdcall VariantToBooleanWithDefault(ptr long)
@ stub VariantToBuffer
@ stub VariantToDosDateTime
@ stdcall VariantToDouble(ptr ptr)
@ stub VariantToDoubleArray
@ stub VariantToDoubleArrayAlloc
@ stdcall VariantToDoubleWithDefault(ptr double)
@ stdcall VariantToFileTime(ptr long ptr)
@ stdcall VariantToGUID(ptr ptr)
@ stdcall VariantToInt16(ptr ptr)
@ stub VariantToInt16Array
@ stub VariantToInt16ArrayAlloc
@ stdcall VariantToInt16WithDefault(ptr long)
@ stdcall VariantToInt32(ptr ptr)
@ stub VariantToInt32Array
@ stub VariantToInt32ArrayAlloc
@ stdcall VariantToInt32WithDefault(ptr long)
@ stdcall VariantToInt64(ptr ptr)
@ stub VariantToInt64Array
@ stub VariantToInt64ArrayAlloc
@ stdcall VariantToInt64WithDefault(ptr int64)
@ stdcall VariantToPropVariant(ptr ptr)
@ stdcall VariantToStrRet(ptr ptr)
@ stdcall VariantToString(ptr ptr long)
@ stdcall VariantToStringAlloc(ptr ptr)
@ stub VariantToStringArray
@ stub VariantToStringArrayAlloc
@ stdcall VariantToStringWithDefault(ptr wstr)
@ stdcall VariantToUInt16(ptr ptr)
@ stub VariantToUInt16Array
@ stub VariantToUInt16ArrayAlloc
@ stdcall VariantToUInt16WithDefault(ptr long)
@ stdcall VariantToUInt32(ptr ptr)
@ stub VariantToUInt32Array
@ stub VariantToUInt32ArrayAlloc
@ stdcall VariantToUInt32WithDefault(ptr long)
@ stdcall VariantToUInt64(ptr ptr)
@ stub VariantToUInt64Array
@ stub VariantToUInt64ArrayAlloc
@ stdcall VariantToUInt64WithDefault(ptr int64)
