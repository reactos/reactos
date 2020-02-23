
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
    support/fxregkey.cpp
    support/fxstringapi.cpp
    support/fxstring.cpp
    support/stringutil.cpp
    support/fxtransactionedlist.cpp
    support/fxcollection.cpp
    support/fxdeviceinterface.cpp
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
    io/fxioqueueapi.cpp
    primitives/mxgeneral.cpp
    irphandlers/fxpkgpnp.cpp
    irphandlers/pnpstatemachine.cpp
    irphandlers/pnpcallbacks.cpp
    irphandlers/eventqueue.cpp
    irphandlers/powerpolicystatemachine.cpp
    irphandlers/fxdefaultirphandler.cpp
    irphandlers/fxpkggeneral.cpp
    irphandlers/fxwmiirphandler.cpp
    irphandlers/fxpkgfdo.cpp
    irphandlers/fdopower.cpp
    irphandlers/poxinterface.cpp
    irphandlers/poweridlestatemachine.cpp
    irphandlers/fxpkgio.cpp
    irphandlers/fxpkgpdo.cpp
    irphandlers/cxpnppowercallbacks.cpp
    irphandlers/powerstatemachine.cpp
    irphandlers/selfmanagediostatemachine.cpp
    irphandlers/fxioqueue.cpp
    irphandlers/pdopower.cpp
    irphandlers/fxwmiprovider.cpp
    irphandlers/notpowerpolicyownerstatemachine.cpp
    targets/fxiotarget.cpp
    targets/fxiotargetself.cpp
)

add_library(wdf01000 MODULE ${SOURCE})

if(MSVC)
    add_target_compile_flags(wdf01000 "/Gz")
endif()

set_module_type(wdf01000 kernelmodedriver)
add_importlibs(wdf01000 ntoskrnl hal wdfldr)
add_cd_file(TARGET wdf01000 DESTINATION reactos/system32/drivers FOR all)