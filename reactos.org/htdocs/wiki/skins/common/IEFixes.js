// IE fixes javascript

var isMSIE55 = (window.showModalDialog && window.clipboardData && window.createPopup);
var doneIETransform;
var doneIEAlphaFix;

if (document.attachEvent)
  document.attachEvent('onreadystatechange', hookit);

function hookit() {
    if (!doneIETransform && document.getElementById && document.getElementById('bodyContent')) {
        doneIETransform = true;
        relativeforfloats();
        fixalpha();
    }
}

// png alpha transparency fixes
function fixalpha() {
    // bg
    if (isMSIE55 && !doneIEAlphaFix)
    {
        var plogo = document.getElementById('p-logo');
        if (!plogo) return;

        var logoa = plogo.getElementsByTagName('a')[0];
        if (!logoa) return;

        var bg = logoa.currentStyle.backgroundImage;
        var imageUrl = bg.substring(5, bg.length-2);

        doneIEAlphaFix = true;

        if (imageUrl.substr(imageUrl.length-4).toLowerCase() == '.png') {
            var logospan = logoa.appendChild(document.createElement('span'));

            logoa.style.backgroundImage = 'none';
            logospan.style.filter = 'progid:DXImageTransform.Microsoft.AlphaImageLoader(src=' + imageUrl + ')';
            logospan.style.height = '100%';
            logospan.style.position = 'absolute';
            logospan.style.width = logoa.currentStyle.width;
            logospan.style.cursor = 'hand';
            // Center image with hack for IE5.5
            if (document.documentElement.dir == "rtl")
            {
              logospan.style.right = '50%';
              logospan.style.setExpression('marginRight', '"-" + (this.offsetWidth / 2) + "px"');
            }
            else
            {
              logospan.style.left = '50%';
              logospan.style.setExpression('marginLeft', '"-" + (this.offsetWidth / 2) + "px"');
            }
            logospan.style.top = '50%';
            logospan.style.setExpression('marginTop', '"-" + (this.offsetHeight / 2) + "px"');

            var linkFix = logoa.appendChild(logoa.cloneNode());
            linkFix.style.position = 'absolute';
            linkFix.style.height = '100%';
            linkFix.style.width = '100%';
        }
    }
}

// fix ie6 disappering float bug
function relativeforfloats() {
    var bc = document.getElementById('bodyContent');
    if (bc) {
        var tables = bc.getElementsByTagName('table');
        var divs = bc.getElementsByTagName('div');
    }
    setrelative(tables);
    setrelative(divs);
}
function setrelative (nodes) {
    var i = 0;
    while (i < nodes.length) {
        if(((nodes[i].style.float && nodes[i].style.float != ('none') ||
        (nodes[i].align && nodes[i].align != ('none'))) &&
        (!nodes[i].style.position || nodes[i].style.position != 'relative'))) 
        {
            nodes[i].style.position = 'relative';
        }
        i++;
    }
}


// Expand links for printing

String.prototype.hasClass = function(classWanted)
{
    var classArr = this.split(/\s/);
    for (var i=0; i<classArr.length; i++)
      if (classArr[i].toLowerCase() == classWanted.toLowerCase()) return true;
    return false;
}

var expandedURLs;

onbeforeprint = function() { 
    expandedURLs = [];

    var contentEl = document.getElementById("content");

    if (contentEl)
    {
      var allLinks = contentEl.getElementsByTagName("a");

      for (var i=0; i < allLinks.length; i++) {
          if (allLinks[i].className.hasClass("external") && !allLinks[i].className.hasClass("free")) {
              var expandedLink = document.createElement("span");
              var expandedText = document.createTextNode(" (" + allLinks[i].href + ")");
              expandedLink.appendChild(expandedText);
              allLinks[i].parentNode.insertBefore(expandedLink, allLinks[i].nextSibling);
              expandedURLs[i] = expandedLink;
          }
      }
   }
}

onafterprint = function()
{
    for (var i=0; i < expandedURLs.length; i++)
        if (expandedURLs[i])
            expandedURLs[i].removeNode(true);
}
