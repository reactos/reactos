
LIST(APPEND SOURCE
#   access.idl
    activaut.idl
#   asynot.idl
#   asysta.idl
    bdaiface.idl
#   binres.idl
    bits.idl
    bits1_5.idl
#   cmdbas.idl
#   cmdtxt.idl
    comcat.idl
    commoncontrols.idl
    control.idl
#   crtrow.idl
    ctxtcall.idl
#   dbccmd.idl
#   dbcses.idl
#   dbdsad.idl
#   dbinit.idl
#   dbprop.idl
#   dbs.idl
    devenum.idl
    dimm.idl
    dispex.idl
    docobj.idl
    downloadmgr.idl
#   dyngraph.idl
    exdisp.idl
    fusion.idl
    hlink.idl
    htiframe.idl
    iads.idl
    icftypes.idl
    imnact.idl
    imnxport.idl
    indexsrv.idl
    mimeinfo.idl
    mimeole.idl
    mscoree.idl
    msdadc.idl
    mshtmhst.idl
    msinkaut.idl
    mstask.idl
    msxml.idl
    msxml2.idl
    netcon.idl
    netfw.idl
    oaidl.idl
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
    shtypes.idl
#   stdole2.idl
    strmif.idl
    structuredquerycondition.idl
    textstor.idl
    tom.idl
    tuner.idl
    unknwn.idl
    urlhist.idl
    urlmon.idl
    vmr9.idl
#   vmrender.idl
    wbemcli.idl
    wia_lh.idl
    wia_xp.idl
    wtypes.idl
    xmldom.idl
    xmldso.idl
    xmllite.idl)

if(NOT MSVC)
  LIST(APPEND SOURCE
    activscp.idl
    activdbg.idl
    asptlb.idl
    ctfutb.idl
    mlang.idl
    msctf.idl
    mshtml.idl
    shobjidl.idl
    wincodec.idl
    wuapi.idl
)
endif()

ADD_INTERFACE_DEFINITIONS(psdk ${SOURCE})

ADD_TYPELIB(stdole2 stdole2.idl)
