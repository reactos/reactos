# This is the list of html and related files packaged in the resource

HTM_FILES = \
            htm\default.hta \
            \
            htm\ad.htm \
            htm\ad2.htm \
            htm\adv.htm \
            htm\as.htm \
            htm\com.htm \
            htm\config.htm \
            htm\cs.htm \
            htm\cs2.htm \
            htm\db.htm \
            htm\details.htm \
            htm\dhcp.htm \
            htm\dhcp2.htm \
            htm\dns.htm \
            htm\dns2.htm \
            htm\email.htm \
            htm\file.htm \
            htm\finish.htm \
            htm\iis.htm \
            htm\iis2.htm \
            htm\intro1.htm \
            htm\intro2.htm \
            htm\intro3.htm \
            htm\menu.htm \
            htm\mq.htm \
            htm\mq2.htm \
            htm\net.htm \
            htm\next.htm \
            htm\oth.htm \
            htm\print.htm \
            htm\ras.htm \
            htm\ras2.htm \
            htm\reg.htm \
            htm\reskit.htm \
            htm\route.htm \
            htm\route2.htm \
            htm\sms.htm \
            htm\sms2.htm \
            htm\top.htm \
            htm\ts.htm \
            htm\ts2.htm \
            htm\web.htm \
            \
            htm\win2000.css \
            \
            htm\menu.js \
            htm\popups.js \
            htm\util.js \
            \
            htm\actvd_bg.gif \
            htm\adv_bg.gif \
            htm\apsrv_bg.gif \
            htm\arrow.gif \
            htm\banner.gif \
            htm\gcold.gif \
            htm\ghot.gif \
            htm\help.gif \
            htm\home_bg.gif \
            htm\mmc.gif \
            htm\mnu_ad1.gif \
            htm\mnu_ad2.gif \
            htm\mnu_adv1.gif \
            htm\mnu_adv2.gif \
            htm\mnu_ap1.gif \
            htm\mnu_ap2.gif \
            htm\mnu_fl1.gif \
            htm\mnu_fl2.gif \
            htm\mnu_hm1.gif \
            htm\mnu_hm2.gif \
            htm\mnu_net1.gif \
            htm\mnu_net2.gif \
            htm\mnu_prt1.gif \
            htm\mnu_prt2.gif \
            htm\mnu_reg1.gif \
            htm\mnu_reg2.gif \
            htm\mnu_web1.gif \
            htm\mnu_web2.gif \
            htm\shadow.gif \
            htm\ntwrk_bg.gif \
            htm\prntr_bg.gif \
            htm\reg2.gif \
            htm\rgstr_bg.gif \
            htm\wbsrv_bg.gif \
            htm\win2kbg.gif \
            htm\wiz.gif \

OTHER_FILES = \
            selfreg.inf   \
            srvwiz.tlb   \

srvwiz.rc: $(HTM_FILES) $(OTHER_FILES)

# iface.h guids.c ctl.tlb : ctl.idl
#            midl /tlb "ctl.tlb" /h "iface.h" /iid "guid.c" ctl.idl 


