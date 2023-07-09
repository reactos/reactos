//menu.js 


// menu items in menu.htm
function reg()
{
  top.content.location.href = "reg.htm";
}

function ad()
{
  top.content.location.href = (srvwiz.DsRole(0)) ? "ad2.htm" :  "ad.htm";
}

function net()
{
  top.content.location.href = "net.htm";
}

function dhcp()
{
  top.content.location.href = (srvwiz.ServiceState("DHCPServer") == 1) ? "dhcp.htm" : "dhcp2.htm";
}

function dns()
{
  top.content.location.href = (srvwiz.ServiceState("DNS") == 1) ? "dns.htm" : "dns2.htm";
}

function ras()
{
  top.content.location.href = (srvwiz.ServiceState("RemoteAccess") == 1) ? "ras.htm" : "ras2.htm";
}

function rtg()
{
  top.content.location.href = (srvwiz.ServiceState("Routing") == 1) ? "route.htm" : "route2.htm";
}

function file()
{
  top.content.location.href = "file.htm";
}

function print()
{
  top.content.location.href = "print.htm";
}

function web()
{
  top.content.location.href = "web.htm";
}

function iis()
{
  top.content.location.href = (srvwiz.ServiceState("IISAdmin") == 1) ? "iis.htm" : "iis2.htm";
}

function sms()
{
  top.content.location.href = (srvwiz.ServiceState("NetShow")  == 1) ? "sms.htm" : "sms2.htm";
}

function as()
{
  top.content.location.href = "as.htm";
}

function com()
{
  top.content.location.href = "com.htm";
}

function db()
{
  top.content.location.href = "db.htm";
}

function mq()
{
  top.content.location.href = (srvwiz.ServiceState("MessageQueue") == 1) ? "mq.htm" : "mq2.htm";
}

function email()
{
  top.content.location.href = "email.htm";
}

function adv()
{
  top.content.location.href = "adv.htm";
}

function cls()
{
  top.content.location.href = (srvwiz.ServiceState("Clustering") > 0) ? "cs.htm" : "cs2.htm";
}

function ts()
{
  top.content.location.href = (srvwiz.ServiceState("TerminalServices") == 1) ? "ts.htm" : "ts2.htm";
}

function reskit()
{
  top.content.location.href = "reskit.htm";
}

function oth()
{
  top.content.location.href = "oth.htm";
}


/*close the open drop-down menu when a different menu item is clicked*/
function MenuOnClick(elem)
{
  var SubMenus = new Array(4);
  var i;

  SubMenus[0] = document.all.n;
  SubMenus[1] = document.all.w;
  SubMenus[2] = document.all.as;
  SubMenus[3] = document.all.a;

  if ( elem == -1 ) { // clcked on a simple menu item
    for ( i = 0; i <= 3; i++ ) { // collapse the expanded sub menu, if any
      if ( SubMenus[i].style.display == '' ) {
        SubMenus[i].style.display = 'none';
      }
    }
  }
  else if ( elem >= 0 && elem <= 3 ){ // clicked on a sub menu
    for ( i = 0; i <= 3; i++ ) {
      if ( elem == i ) { // toggle the clicked sub menu
        if ( SubMenus[i].style.display == '' ) {
          SubMenus[i].style.display = 'none';
        } else {
          SubMenus[i].style.display = '';
        }
      } else { // and collapse all others
        if ( SubMenus[i].style.display == '' ) {
          SubMenus[i].style.display = 'none';
        }
      }
    }
  }
  else { 
    // invalid argument
  }
}


/*swap images to left of menu onmouseover*/
function b_on(n)
{
  if (n==1)  { document["b1"].src = "mnu_hm2.gif";    }
  if (n==2)  { document["b2"].src = "mnu_reg2.gif";   }
  if (n==3)  { document["b3"].src = "mnu_ad2.gif";    }
  if (n==4)  { document["b4"].src = "mnu_net2.gif";   }
  if (n==5)  { document["b5"].src = "mnu_fl2.gif";    }
  if (n==6)  { document["b6"].src = "mnu_prt2.gif";   }
  if (n==7)  { document["b7"].src = "mnu_web2.gif";   }
  if (n==8)  { document["b8"].src = "mnu_ap2.gif";    }
  if (n==9)  { document["b9"].src = "mnu_adv2.gif";   }
  if (n==10) { document["b10"].src = "mnu_fin2.gif";  }
}
   
function b_off(n)
{
  if (n==1)  { document["b1"].src = "mnu_hm1.gif";    }
  if (n==2)  { document["b2"].src = "mnu_reg1.gif";   }
  if (n==3)  { document["b3"].src = "mnu_ad1.gif";    }
  if (n==4)  { document["b4"].src = "mnu_net1.gif";   }
  if (n==5)  { document["b5"].src = "mnu_fl1.gif";    }
  if (n==6)  { document["b6"].src = "mnu_prt1.gif";   }
  if (n==7)  { document["b7"].src = "mnu_web1.gif";   }
  if (n==8)  { document["b8"].src = "mnu_ap1.gif";    }
  if (n==9)  { document["b9"].src = "mnu_adv1.gif";   }
  if (n==10) { document["b10"].src = "mnu_fin1.gif";  }
}
