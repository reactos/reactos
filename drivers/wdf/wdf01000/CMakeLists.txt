
include_directories(
    include)
#    ${REACTOS_SOURCE_DIR}/sdk/include/wdf/kmdf/1.9)

add_definitions(-D__WDF_MAJOR_VERSION=1 -D__WDF_MINOR_VERSION=9 -D__WDF_BUILD_NUMBER=7600 -DWDF_DEVICE_NO_WDMSEC_H -DFX_CORE_MODE=1 -DFX_CORE_KERNEL_MODE=1 -DFX_IS_KERNEL_MODE=1 -DDBG)

list(APPEND SOURCE
    wdf01000.cpp
    librarycommon/fxlibrarycommon.cpp
    object/globals.cpp
    object/fxverifierlock.cpp
    object/wdfpool.cpp
    object/dbgtrace.cpp
    object/fxobject.cpp
    object/fxobjectapi.cpp
    object/fxvalidatefunctions.cpp
    object/fxverifierbugcheck.cpp
    object/handleapi.cpp
    object/fxtagtracker.cpp
    object/fxobjectstatemachine.cpp
    object/fxuserobjectapi.cpp
    support/fxregkey.cpp
    support/fxstringapi.cpp
    support/fxstring.cpp
    support/stringutil.cpp
    support/fxtransactionedlist.cpp
    support/fxcollection.cpp
    support/fxcollectionapi.cpp
    support/fxdeviceinterface.cpp
    support/fxdeviceinterfaceapi.cpp
    support/fxspinlockapi.cpp
    support/fxspinlock.cpp
    support/fxresourcecollection.cpp
    support/fxqueryinterface.cpp
    support/fxresourceapi.cpp
    support/fxregistryapi.cpp
    support/fxwaitlockapi.cpp
    support/fxqueryinterfaceapi.cpp
    core/fxbugcheckcallback.cpp
    core/tracing.cpp
    core/fxdriver.cpp
    core/fxdriverapi.cpp
    core/fxdeviceapi.cpp
    core/fxdeviceinit.cpp
    core/fxdeviceinitapi.cpp
    core/fxdeviceinterfaceapi.cpp
    core/fxtimerapi.cpp
    core/fxrequestapi.cpp
    core/fxmemorybufferapi.cpp
    core/verifierapi.cpp
    core/fxdisposelist.cpp
    core/fxsystemworkitem.cpp
    core/fxdevice.cpp
    core/fxdevicebase.cpp
    core/fxsystemthread.cpp
    core/fxchildlist.cpp
    core/fxrequestbase.cpp
    core/fxrequest.cpp
    core/fxirpqueue.cpp
    core/fxfileobject.cpp
    core/fxrequestoutputbuffer.cpp
    core/fxrequestsystembuffer.cpp
    core/fxtimer.cpp
    core/fxmemoryobject.cpp
    core/fxchildlistapi.cpp
    core/fxdevicecontrolapi.cpp
    core/fxdpcapi.cpp
    core/fxdevicefdoapi.cpp
    core/fxfileobjectapi.cpp
    core/fxmemorybufferreallocatedapi.cpp
    core/fxlookasidelistapi.cpp
    core/fxdevicepdoapi.cpp
    core/fxworkitemapi.cpp
    primitives/mxgeneral.cpp
    irphandlers/pnp/fxpkgpnp.cpp
    irphandlers/pnp/pnpstatemachine.cpp
    irphandlers/pnp/pnpcallbacks.cpp
    irphandlers/pnp/eventqueue.cpp
    irphandlers/pnp/powerpolicystatemachine.cpp
    irphandlers/pnp/fxpkgfdo.cpp
    irphandlers/pnp/fdopower.cpp
    irphandlers/pnp/poxinterface.cpp
    irphandlers/pnp/poweridlestatemachine.cpp
    irphandlers/pnp/fxpkgpdo.cpp
    irphandlers/pnp/cxpnppowercallbacks.cpp
    irphandlers/pnp/powerstatemachine.cpp
    irphandlers/pnp/selfmanagediostatemachine.cpp
    irphandlers/pnp/pdopower.cpp
    irphandlers/pnp/notpowerpolicyownerstatemachine.cpp
    irphandlers/pnp/interruptobject.cpp
    irphandlers/pnp/support.cpp
    irphandlers/pnp/fxinterruptapi.cpp
    irphandlers/default/fxdefaultirphandler.cpp
    irphandlers/general/fxpkggeneral.cpp
    irphandlers/io/fxpkgio.cpp
    irphandlers/io/fxioqueue.cpp
    irphandlers/io/fxioqueueapi.cpp
    irphandlers/wmi/fxwmiirphandler.cpp
    irphandlers/wmi/fxwmiprovider.cpp
    targets/fxiotarget.cpp
    targets/fxiotargetself.cpp
    targets/general/fxiotargetapi.cpp
    dma/fxdmaenabler.cpp
    dma/fxcommonbufferapi.cpp
    dma/fxdmaenablerapi.cpp
    dma/fxdmatransactionapi.cpp
)

add_library(wdf01000 MODULE ${SOURCE})

if(MSVC)
    add_target_compile_flags(wdf01000 "/Gz")
endif()

set_module_type(wdf01000 kernelmodedriver)
add_importlibs(wdf01000 ntoskrnl hal wdfldr)
add_cd_file(TARGET wdf01000 DESTINATION reactos/system32/drivers FOR all)
