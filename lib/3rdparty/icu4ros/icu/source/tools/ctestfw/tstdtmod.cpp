/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2002-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/* Created by weiv 05/09/2002 */

#include "unicode/tstdtmod.h"
#include "cmemory.h"

TestLog::~TestLog() {}

TestDataModule *TestDataModule::getTestDataModule(const char* name, TestLog& log, UErrorCode &status)
{
  if(U_FAILURE(status)) {
    return NULL;
  }
  TestDataModule *result = NULL;

  // TODO: probe for resource bundle and then for XML.
  // According to that, construct an appropriate driver object

  result = new RBTestDataModule(name, log, status);
  if(U_SUCCESS(status)) {
    return result;
  } else {
    delete result;
    return NULL;
  }
}

TestDataModule::TestDataModule(const char* name, TestLog& log, UErrorCode& /*status*/)
: testName(name),
fInfo(NULL),
fLog(log)
{
}

TestDataModule::~TestDataModule() {
  if(fInfo != NULL) {
    delete fInfo;
  }
}

const char * TestDataModule::getName() const
{
  return testName;
}



RBTestDataModule::~RBTestDataModule()
{
  ures_close(fTestData);
  ures_close(fModuleBundle);
  ures_close(fInfoRB);
  uprv_free(tdpath);
}

RBTestDataModule::RBTestDataModule(const char* name, TestLog& log, UErrorCode& status) 
: TestDataModule(name, log, status),
  fModuleBundle(NULL),
  fTestData(NULL),
  fInfoRB(NULL),
  tdpath(NULL)
{
  fNumberOfTests = 0;
  fDataTestValid = TRUE;
  fModuleBundle = getTestBundle(name, status);
  if(fDataTestValid) {
    fTestData = ures_getByKey(fModuleBundle, "TestData", NULL, &status);
    fNumberOfTests = ures_getSize(fTestData);
    fInfoRB = ures_getByKey(fModuleBundle, "Info", NULL, &status);
    if(status != U_ZERO_ERROR) {
      log.errln(UNICODE_STRING_SIMPLE("Unable to initalize test data - missing mandatory description resources!"));
      fDataTestValid = FALSE;
    } else {
      fInfo = new RBDataMap(fInfoRB, status);
    }
  }
}

UBool RBTestDataModule::getInfo(const DataMap *& info, UErrorCode &/*status*/) const
{
    info = fInfo;
    if(fInfo) {
        return TRUE;
    } else {
        return FALSE;
    }
}

TestData* RBTestDataModule::createTestData(int32_t index, UErrorCode &status) const 
{
  TestData *result = NULL;
  UErrorCode intStatus = U_ZERO_ERROR;

  if(fDataTestValid == TRUE) {
    // Both of these resources get adopted by a TestData object.
    UResourceBundle *DataFillIn = ures_getByIndex(fTestData, index, NULL, &status); 
    UResourceBundle *headers = ures_getByKey(fInfoRB, "Headers", NULL, &intStatus);
  
    if(U_SUCCESS(status)) {
      result = new RBTestData(DataFillIn, headers, status);

      if(U_SUCCESS(status)) {
        return result;
      } else {
        delete result;
      }
    } else {
      ures_close(DataFillIn);
      ures_close(headers);
    }
  } else {
    status = U_MISSING_RESOURCE_ERROR;
  }
  return NULL;
}

TestData* RBTestDataModule::createTestData(const char* name, UErrorCode &status) const
{
  TestData *result = NULL;
  UErrorCode intStatus = U_ZERO_ERROR;

  if(fDataTestValid == TRUE) {
    // Both of these resources get adopted by a TestData object.
    UResourceBundle *DataFillIn = ures_getByKey(fTestData, name, NULL, &status); 
    UResourceBundle *headers = ures_getByKey(fInfoRB, "Headers", NULL, &intStatus);
   
    if(U_SUCCESS(status)) {
      result = new RBTestData(DataFillIn, headers, status);
      if(U_SUCCESS(status)) {
        return result;
      } else {
        delete result;
      }
    } else {
      ures_close(DataFillIn);
      ures_close(headers);
    }
  } else {
    status = U_MISSING_RESOURCE_ERROR;
  }
  return NULL;
}



//Get test data from ResourceBundles
UResourceBundle* 
RBTestDataModule::getTestBundle(const char* bundleName, UErrorCode &status) 
{
  if(U_SUCCESS(status)) {
    UResourceBundle *testBundle = NULL;
    const char* icu_data = fLog.getTestDataPath(status);
    if (testBundle == NULL) {
        testBundle = ures_openDirect(icu_data, bundleName, &status);
        if (status != U_ZERO_ERROR) {
            fLog.errln(UNICODE_STRING_SIMPLE("Failed: could not load test data from resourcebundle: ") + UnicodeString(bundleName, -1, US_INV));
            fDataTestValid = FALSE;
        }
    }
    return testBundle;
  } else {
    return NULL;
  }
}

