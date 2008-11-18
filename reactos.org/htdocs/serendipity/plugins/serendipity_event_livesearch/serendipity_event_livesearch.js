/*
// +----------------------------------------------------------------------+
// | Copyright (c) 2004 Bitflux GmbH                                      |
// +----------------------------------------------------------------------+
// | Licensed under the Apache License, Version 2.0 (the "License");      |
// | you may not use this file except in compliance with the License.     |
// | You may obtain a copy of the License at                              |
// | http://www.apache.org/licenses/LICENSE-2.0                           |
// | Unless required by applicable law or agreed to in writing, software  |
// | distributed under the License is distributed on an "AS IS" BASIS,    |
// | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or      |
// | implied. See the License for the specific language governing         |
// | permissions and limitations under the License.                       |
// +----------------------------------------------------------------------+
// | Author: Bitflux GmbH <devel@bitflux.ch>                              |
// +----------------------------------------------------------------------+

*/
var liveSearchReq = false;
var t = null;
var liveSearchLast = "";
var isIE = false;

// on !IE we only have to initialize it once
if (window.XMLHttpRequest) {
    liveSearchReq = new XMLHttpRequest();
}

function addLoadEvent(func) {
  var oldonload = window.onload;
  if (typeof window.onload != 'function') {
    window.onload = func;
  } else {
    window.onload = function() {
      oldonload();
      func();
    }
  }
}

function liveSearchInit() {
    if (navigator.userAgent.indexOf("Safari") > 0) {
        document.getElementById('serendipityQuickSearchTermField').addEventListener("keydown", liveSearchKeyPress,false);
        document.getElementById('searchform').addEventListener("submit", liveSearchSubmit,false);
    } else if (navigator.product == "Gecko") {
        document.getElementById('serendipityQuickSearchTermField').addEventListener("keypress", liveSearchKeyPress,false);
        document.getElementById('searchform').addEventListener("submit", liveSearchSubmit,false);
    } else {
        document.getElementById('serendipityQuickSearchTermField').attachEvent("onkeydown", liveSearchKeyPress);
        document.getElementById('searchform').attachEvent("onsubmit", liveSearchSubmit);
        isIE = true;
    }

    if (document.getElementById('searchform').setAttribute) {
        document.getElementById('searchform').setAttribute('autocomplete','off');
        document.getElementById('serendipityQuickSearchTermField').setAttribute('autocomplete','off');
    }
    document.getElementById('serendipityQuickSearchTermField').style.border = '1px solid green';
}

function liveSearchKeyPress(event) {
    if (event.keyCode == 40 ) { //KEY DOWN
        highlight = document.getElementById("LSHighlight");
        if (!highlight) {
            highlight = document.getElementById("LSResult").firstChild.firstChild.firstChild;
        } else {
            highlight.removeAttribute("id");
            highlight = highlight.nextSibling;
        }
        if (highlight) {
            highlight.setAttribute("id","LSHighlight");
        }
        if (!isIE) { event.preventDefault(); }
    }
    //KEY UP
    else if (event.keyCode == 38 ) {
        highlight = document.getElementById("LSHighlight");
        if (!highlight) {
            highlight = document.getElementById("LSResult").firstChild.firstChild.lastChild;
        }
        else {
            highlight.removeAttribute("id");
            highlight = highlight.previousSibling;
        }
        if (highlight) {
                highlight.setAttribute("id","LSHighlight");
        }
        if (!isIE) { event.preventDefault(); }
    }
    //ESC
    else if (event.keyCode == 27) {
        highlight = document.getElementById("LSHighlight");
        if (highlight) {
            highlight.removeAttribute("id");
        }
        document.getElementById("LSResult").style.display = "none";
    } else {
        liveSearchStart();
    }
}
function liveSearchStart() {
    if (t) {
        window.clearTimeout(t);
    }
    t = window.setTimeout("liveSearchDoSearch()",200);
}

function liveSearchDoSearch() {
    v = document.getElementById('serendipityQuickSearchTermField').value;
    if (liveSearchLast != v && v.length > 3) {
        if (liveSearchReq && liveSearchReq.readyState < 4) {
            liveSearchReq.abort();
        }

        if (v == "") {
            document.getElementById("LSResult").style.display = "none";
            highlight = document.getElementById("LSHighlight");
            if (highlight) {
                highlight.removeAttribute("id");
            }
            return false;
        }

        if (window.XMLHttpRequest) {
        // branch for IE/Windows ActiveX version
        } else if (window.ActiveXObject) {
            liveSearchReq = new ActiveXObject("Microsoft.XMLHTTP");
        }

        document.getElementById('LSResult').style.display = "block";
        document.getElementById('LSResult').firstChild.innerHTML = '<div class="serendipity_livesearch_result">' + waittext + '</div>';

        liveSearchReq.onreadystatechange= liveSearchProcessReqChange;
        liveSearchReq.open("GET", lsbase + "s=" + v);
        liveSearchLast = v;
        liveSearchReq.send(null);
    }
}

function liveSearchProcessReqChange() {

    if (liveSearchReq.readyState == 4) {
        var  res = document.getElementById("LSResult");
        res.style.display = "block";
        res.firstChild.innerHTML = liveSearchReq.responseText;
    }
}

function liveSearchSubmit() {
    var highlight = document.getElementById("LSHighlight");
    if (highlight && highlight.firstChild) {
        document.getElementById('searchform').action = highlight.firstChild.getAttribute("href");
        return false;
    } else {
        return true;
    }
}
