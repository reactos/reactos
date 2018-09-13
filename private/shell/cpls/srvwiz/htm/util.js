// util.js

var shapp = new ActiveXObject("Shell.Application");
var srvwiz = new ActiveXObject("SrvWiz.SrvWiz");

document.oncontextmenu = fnContextMenu;

function fnContextMenu()
{
   event.returnValue = false;
}


function exec(cmd, arg)
{
    shapp.ShellExecute(cmd, arg);
}

function help(arg)
{
    exec('hh.exe', arg);
}

function SHGetSpecialFolderPath(cidl)
{
    var shfdr = shapp.NameSpace(cidl);
    var shfdritm = shfdr.Self;

    return shfdritm.Path;
}

function home()
{
    var home = srvwiz.Home;

    if (home == 0)
    {
        top.content.location.href = "finish.htm";
    }
    else if ((home >= 1) && (home <= 5))
    {
        top.content.location.href = "intro1.htm";
    }
    else
    {
        top.content.location.href = "config.htm";
    }

    // set focus to content frame
    top.content.focus();
}




