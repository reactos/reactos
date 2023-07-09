var timerID = 0;
var displayedItem;
var LHeader = "Welcome";
var LBody = "Welcome to the exciting new world of Windows NT 5.0, where your computer desktop meets the Internet!<p>Sit back and relax as you take a brief tour of the options available on this screen.<p>If you want to explore an option, just click it.";
var aBitmaps = new Array( "computer.gif", "register.gif", "internet.gif", "disk.gif");

function Select()
{
  var target = window.event.srcElement;

  var newImage;
  var name;
  var body;

  if ( timerID )
  {
    window.clearTimeout(timerID);
    timerID = 0;
  }

  if (displayedItem)
  {
    displayedItem.style.backgroundColor = "#CEDFEF";
  }

  if ( (target.imgindex >= 0) && (target.imgindex < 4) )
  {
    newImage = "URL(res://welcome.exe/"+aBitmaps[target.imgindex]+")";
  }
  else
  {
    newImage = "URL(res://welcome.exe/computer.gif)";
  }

  target.style.backgroundColor = "#E6F0F9";
  header.innerText = target.header;
  description.innerHTML = target.body;
  rightpanel.style.backgroundImage = newImage;
  displayedItem = target;
}
function ReturnToDefault()
{
  if (displayedItem)
  {
    displayedItem.style.backgroundColor = "#CEDFEF";
    displayedItem = null;
  }
  header.innerText = LHeader;
  description.innerHTML = LBody;
  rightpanel.style.backgroundImage = "URL(res://welcome.exe/computer.gif)";
  timerID = 0;
}
function TimedSelect()
{
  timerID = window.setTimeout( "ReturnToDefault();", 100 );
}
function Click()
{
  ExecItem( window.event.srcElement );
}
function KeyDown()
{
  var code = window.event.keyCode;

  if ( code == 0x0d || code == 0x20 )
  {
    ExecItem( window.event.srcElement );
  }
}
function ExecItem( target )
{
  target.style.color = "gray";
  window.navigate("app://"+target.appindex);
}
function SetContentHeight()
{
  var cyBody = document.body.offsetHeight;
  contents.style.pixelHeight = cyBody - 113;
  checkboxarea.style.pixelTop = cyBody - 115 - checkboxarea.offsetHeight;
}
function EatErrors()
{
  return true;
}