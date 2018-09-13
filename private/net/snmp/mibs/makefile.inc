MIBCC = mibcc.exe

MIB_COMPONENTS = \
    mib_ii.mib \
    ipforwd.mib \
    lmmib2.mib \
    msft.mib \
    wins.mib \
    dhcp.mib \
    inetsrv.mib \
    http.mib \
    gopherd.mib \
    ftp.mib \
    mipx.mib \
    mripsap.mib \
    acs.mib	\
    printmib.mib \
    igmpv2.mib \
    hostmib.mib \
    authserv.mib \
    accserv.mib \
    mcastmib.mib

$(O)\mib.bin: $(MIB_COMPONENTS)
    $(MIBCC) -o$(O)\mib.bin $**
