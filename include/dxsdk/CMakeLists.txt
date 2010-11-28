
LIST(APPEND SOURCE
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
    strmif.idl
    tvratings.idl
    vidcap.idl
    videoacc.idl
    #vmrender.idl
)

# HACK: dxsdk is broken
    ADD_INTERFACE_DEFINITIONS(dxsdk ${SOURCE})

