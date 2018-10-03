
add_definitions(-D_MIDL_USE_GUIDDEF_)

list(APPEND IDL_SOURCES
# These are the ones that MS uuid contains:
#    activaut.idl
    activdbg.idl
    activscp.idl
#    alg.idl
#    autodiscovery.idl
#    bidispl.idl
#    bidisplp.idl
#    bthguid
#    callobj.idl
#    cguid.idl
#    chanmgr.idl
#    ciintf.idl
#    cladmwiz.idl
#    cluadmex.idl
#    cluscfgserver.idl
#    cluscfgwizard.idl
    comcat.idl
    commoncontrols.idl
#    confpriv.idl
#    ctffunc.idl
#    ctfbui.idl
#    ctfspui.idl
#    ctfutb.idl
    ctxtcall.idl
#    cxq_cust.idl
#    dbgprop.idl
    dimm.idl
    dispex.idl
#    dlmgridl.idl
    docobj.idl
#    dwnnot.idl
#    dxtmsft.idl
#    dxtrans.idl
#    dxtransp.idl
#    emptyvc.idl
    exdisp.idl
#    filter.idl
#    filtntfy.idl
#    fsciclnt.idl
#    h323priv.idl
    hlink.idl
#    hnetbcon.idl
#    hnetcfg.idl
    htiface.idl
    htiframe.idl
#    htmlfilter.idl
    httprequest.idl
#    iaccess.idl
#    iaspolcy.idl
#    iedial.idl
#    iextag.idl
#    imapi.idl
#    imguitil.idl
#    immact.idl
#    indexsrc.idl
#    inputscope.idl
#    interned.idl
#    ipmsp.idl
#    mdhcp.idl
    mimeinfo.idl
    mlang.idl
#    mobsync.idl
#    mobsyncp.idl
#    mparser.idl
    msctf.idl
#    msdaosp.idl
    mshtmhst.idl
    mshtml.idl
#    msp.idl
#    mstime.idl
    msxml.idl
#    my_msdatsrc.idl
    natupnp.idl
#    netcfgn.idl
#    netcfgp.idl
#    netcfgx.idl
    netcon.idl
#    netconp.idl
#    netfwv6.idl
#    netfwv6clsid
#    netmon.idl
#    netprov.idl
    oaidl.idl
    objidl.idl
    objsafe.idl
    ocidl.idl
    ocmm.idl
    oleacc.idl
#    oleext.idl
    oleidl.idl
#    opsprof.idl
    optary.idl
#    pbagex.idl
    perhist.idl
#    prgsnk.idl
    propidl.idl
#    query.idl
#    ratingsp.idl
#    recalc.idl
#    rend.idl
#    rtccore.idl
#    safeocx.idl
#    sdoias.idl
#    sdpblb.idl
    sensevts.idl
    servprov.idl
#    shappmgr.idl
    shdeprecated.idl
#    shgina.idl
#    shimgdata.idl
    shldisp.idl
    shobjidl.idl
#    sifmt.idl
#    simpdata.idl
#    softkbd.idl
#    storext.idl
#    subsmgr.idl
#    syncmgr.idl
#    sysmgmt.idl
#    tapi3.idl
#    tapi3if.idl
#    termmgr.idl
    textstor.idl
#    tlog.idl
    tlogstg.idl
#    trkadm.idl
#    trkcom.idl
    unknwn.idl
#    upnp.idl
#    upnphost.idl
#    upnpp.idl
    urlhist.idl
    urlmon.idl
#    urltrack.idl
#    vervec.idl
#    vrsscan.idl
#    webcheck.idl
#    xenroll.idl
#    xmlparser.idl
#    xmlprov.idl

# These are others that reactos provides:
    asptlb.idl
    bdaiface.idl
    bits.idl
    bits1_5.idl
    control.idl
    docobjectservice.idl
    downloadmgr.idl
    fusion.idl
    iads.idl
    imnact.idl
    imnxport.idl
    indexsrv.idl
    mimeole.idl
    mscoree.idl
    msdadc.idl
    mstask.idl
    netfw.idl
    oledb.idl
    propsys.idl
    pstore.idl
    richole.idl
    strmif.idl
    tom.idl
    tuner.idl
    vmr9.idl
    wia_xp.idl
    wincodec.idl
    xmldom.idl
    xmldso.idl
    ctfutb.idl
    xmllite.idl)

if(MSVC)
    list(APPEND IDL_SOURCES
        atliface.idl
        cor.idl
        cordebug.idl
        metahost.idl)
else()
    list(APPEND IDL_SOURCES
        wbemcli.idl
        wuapi.idl)
endif()

foreach(IDL_FILE ${IDL_SOURCES})
    get_filename_component(NAME ${IDL_FILE} NAME_WE)
    generate_idl_iids(${CMAKE_SOURCE_DIR}/sdk/include/psdk/${IDL_FILE})
    list(APPEND SOURCE ${NAME}_i.c)
endforeach()

list(APPEND SOURCE
    cguid_i.c
#    bthguid.c
#    cluscfgguids.c
    cmdtreeiid.c
#    ctffunc_g.c
#    ctfspui_g.c
#    hnclsid.c
    ieguids.c
#    iid.c
#    msctf_g.c
#    ncclsid.c
#    ndisguid.c
    netfwv6clsid.c
#    oledbdat.c
#    oledbdepiid.c
#    oledbiid.c
#    oledbnewiid.c
#    qosguid.c
    shguids.c
#    tapi3iid.c
#    vcguids.c
    wdmguids.c
    otherguids.c
    undoc.c
    precomp.h)

add_library(uuid ${SOURCE})
add_pch(uuid precomp.h SOURCE)
add_dependencies(uuid psdk)
