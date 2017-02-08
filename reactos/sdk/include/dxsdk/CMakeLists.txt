
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

list(APPEND SOURCE
    amstream.idl
    amvideo.idl
    austream.idl
    #axcore.idl
    #axextend.idl
    bdatif.idl
    ddstream.idl
    devenum.idl
    dmodshow.idl
    #dshowasf.idl
    #dyngraph.idl
    mediaobj.idl
    medparam.idl
    mixerocx.idl
    mmstream.idl
    qedit.idl
    regbag.idl
    tvratings.idl
    vidcap.idl
    videoacc.idl
    #vmrender.idl
)

add_idl_headers(dxsdk ${SOURCE})

add_iid_library(mediaobj_guid mediaobj.idl)
