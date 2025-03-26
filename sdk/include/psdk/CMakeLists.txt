
add_definitions(-D_MIDL_USE_GUIDDEF_)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND SOURCE
#   access.idl
    activaut.idl
    activdbg.idl
    activscp.idl
    asptlb.idl
#   asynot.idl
#   asysta.idl
    atliface.idl
    audioclient.idl
    audiopolicy.idl
    bdaiface.idl
#   binres.idl
    bits.idl
    bits1_5.idl
    bits2_0.idl
    bits2_5.idl
    bits3_0.idl
#   cmdbas.idl
#   cmdtxt.idl
    comcat.idl
    commoncontrols.idl
    control.idl
    cor.idl
    cordebug.idl
#   crtrow.idl
    ctffunc.idl
    ctxtcall.idl
#   dbccmd.idl
#   dbcses.idl
#   dbdsad.idl
#   dbinit.idl
#   dbprop.idl
#   dbs.idl
    devenum.idl
    devicetopology.idl
    dimm.idl
    dispex.idl
    docobj.idl
    docobjectservice.idl
    downloadmgr.idl
    drmexternals.idl
    dxgiformat.idl
#   dyngraph.idl
    endpointvolume.idl
    exdisp.idl
    fusion.idl
    hlink.idl
    htiface.idl
    htiframe.idl
    httprequest.idl
    iads.idl
    icftypes.idl
    ieautomation.idl
    imnact.idl
    imnxport.idl
    indexsrv.idl
    inputscope.idl
    inseng.idl
    mediaobj.idl
    metahost.idl
    mfidl.idl
    mfobjects.idl
    mftransform.idl
    mimeinfo.idl
    mimeole.idl
    mlang.idl
    mmdeviceapi.idl
    mscoree.idl
    msctf.idl
    msdadc.idl
    mshtmhst.idl
    mshtml.idl
    msinkaut.idl
    mstask.idl
    msxml.idl
    msxml2.idl
    msxml6.idl
    natupnp.idl
    netcon.idl
    netfw.idl
    oaidl.idl
    objectarray.idl
    objidl.idl
    objsafe.idl
    ocidl.idl
    ocmm.idl
    oleacc.idl
    oledb.idl
    oleidl.idl
#   opnrst.idl
    optary.idl
    perhist.idl
    propidl.idl
    propsys.idl
    pstore.idl
    richole.idl
#   row.idl
#   rowchg.idl
#   rstbas.idl
#   rstinf.idl
#   rstloc.idl
    sensevts.idl
    servprov.idl
#   sesprp.idl
    shdeprecated.idl
    shldisp.idl
    shobjidl.idl
    shtypes.idl
#   stdole2.idl
    strmif.idl
    structuredquerycondition.idl
    taskschd.idl
    textstor.idl
    tlogstg.idl
    tom.idl
    transact.idl
    tuner.idl
    txcoord.idl
    txdtc.idl
    unknwn.idl
    urlhist.idl
    urlmon.idl
    vmr9.idl
#   vmrender.idl
    vsbackup.idl
    vss.idl
    vswriter.idl
    wbemcli.idl
    wbemdisp.idl
    wbemprov.idl
    wia_lh.idl
    wia_xp.idl
    wincodec.idl
    wincodecsdk.idl
    winsxs.idl
    wmdrmsdk.idl
    wmiutils.idl
    wmsbuffer.idl
    wmsdkidl.idl
    wtypes.idl
    wuapi.idl
    xmldom.idl
    xmldso.idl
    ctfutb.idl
    xmllite.idl)

add_idl_headers(psdk ${SOURCE})
add_dependencies(psdk xdk)
add_typelib(stdole2.idl)
add_custom_target(stdole2 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stdole2.tlb)

add_idl_headers(d3d_idl_headers d3dcommon.idl)

add_iid_library(adsiid iads.idl transact.idl)
add_iid_library(wuguid wuapi.idl)
add_iid_library(xml_uuids msxml2.idl)

generate_idl_iids(wincodec.idl)
