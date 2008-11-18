<!-- // Hide from older browsers
/* $Id: serendipity_editor.js 556 2005-10-14 10:37:53Z garvinhicking $ */
/*
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details
*/

/*
    Written by chris wetherell
    http://www.massless.org
    chris [THE AT SIGN] massless.org

    warning: it only works for IE4+/Win and Moz1.1+
    feel free to take it for your site
    if there are any problems, let chris know.
*/

var thisForm;

function getMozSelection(txtarea) {
    var selLength = txtarea.textLength;
    var selStart = txtarea.selectionStart;
    var selEnd = txtarea.selectionEnd;

    if (selEnd==1 || selEnd==2) {
        selEnd=selLength;
    }
    return (txtarea.value).substring(selStart, selEnd);
}

function getIESelection(txtarea) {
    return document.selection.createRange().text;
}

function mozWrap(txtarea, lft, rgt) {
    var selLength = txtarea.textLength;
    var selStart = txtarea.selectionStart;
    var selEnd = txtarea.selectionEnd;

    if (selEnd==1 || selEnd==2) selEnd=selLength;
    var s1 = (txtarea.value).substring(0,selStart);
    var s2 = (txtarea.value).substring(selStart, selEnd)
    var s3 = (txtarea.value).substring(selEnd, selLength);
    txtarea.value = s1 + lft + s2 + rgt + s3;
}

function IEWrap(txtarea, lft, rgt) {
    strSelection = document.selection.createRange().text;
    if (strSelection != "") {
        document.selection.createRange().text = lft + strSelection + rgt;
    } else {
        txtarea.value = txtarea.value + lft + rgt;
    }
}

function wrapSelection(txtarea, lft, rgt) {
    if (document.all) {
        IEWrap(txtarea, lft, rgt);
    } else if (document.getElementById) {
        mozWrap(txtarea, lft, rgt);
    }
}

function wrapSelectionWithLink(txtarea) {
    var my_link = prompt("Enter URL:","http://");

    if (document.all && getIESelection(txtarea) == "" ||
         document.getElementById && getMozSelection(txtarea) == "") {
        var my_desc = prompt("Enter Description", '');
    }
    
    var my_title = prompt("Enter title/tooltip:", "");

	html_title = "";
    if (my_title != "") {
		html_title = ' title="' + my_title + '"';
    }

    if (my_link != null) {
        lft = "<a href=\"" + my_link + "\" " + html_title + ">";
        if (my_desc != null && my_desc != "") {
            rgt = my_desc + "</a>";
        } else {
            rgt = "</a>";
        }
        wrapSelection(txtarea, lft, rgt);
    }

    return;
}
/* end chris w. script */

function mozInsert(txtarea, str) {
    var selLength = txtarea.textLength;
    var selStart = txtarea.selectionStart;
    var selEnd = txtarea.selectionEnd;
    if (selEnd==1 || selEnd==2) {
        selEnd=selLength;
    }
    var s1 = (txtarea.value).substring(0,selStart);
    var s2 = (txtarea.value).substring(selStart, selEnd)
    var s3 = (txtarea.value).substring(selEnd, selLength);
    txtarea.value = s1 + str + s2 + s3;
}

function wrapInsImage(area) {
    var loc = prompt('Enter the Image Location: ');
    if (!loc) {
        return;
    }
    mozInsert(area,'<img src="'+ loc + '" alt="" />');
}

/* end Better-Editor functions */

function serendipity_insImage (area) {
    var loc = prompt('Enter the Image Location: ');
    if (!loc) {
        area.focus();
        return;
    }

    area.value = area.value + '<img src="' + loc + '" alt="" />';
    area.focus();
}

function serendipity_insBasic (area, tag) {
    area.value = area.value + "<" + tag + "></" + tag + ">";
    area.focus();
}

function serendipity_insLink (area) {
    var loc      = prompt('Enter URL Location: ');
    var text     = prompt('Enter Description: ');
    var my_title = prompt("Enter title/tooltip:", "");

    if (!loc) {
        area.focus();
        return;
    }
    
	html_title = "";
    if (my_title != "") {
		html_title = ' title="' + my_title + '"';
    }

    area.value = area.value + '<a href="' + loc + '" ' + html_title + '>' + (text ? text : loc) + '</a>';
    area.focus();
}

function serendipity_imageSelector_addToElement (str, el)
{
    document.getElementById(el).value = str;
    document.getElementById(el).focus();
    if (document.getElementById(el).onchange) {
        document.getElementById(el).onchange();
    }
}

function serendipity_imageSelector_addToBody (str, textarea)
{
    eltarget = '';
    if (document.forms['serendipityEntry'] && document.forms['serendipityEntry']['serendipity['+ textarea +']']) {
        eltarget = document.forms['serendipityEntry']['serendipity['+ textarea +']']
    } else if (document.forms['serendipityEntry'] && document.forms['serendipityEntry'][textarea]) {
        eltarget = document.forms['serendipityEntry'][textarea];
    } else {
        eltarget = document.forms[0].elements[0];
    }

	wrapSelection(eltarget, str, '');
    eltarget.focus();
}

function serendipity_imageSelector_done(textarea)
{
    var insert = '';
    var img = '';
    var src = '';
    var f = document.forms['serendipity[selForm]'].elements;

    if (f['serendipity[linkThumbnail]'][0].checked == true) {
        img       = f['thumbName'].value;
        imgWidth  = f['imgThumbWidth'].value;
        imgHeight = f['imgThumbHeight'].value;
    } else {
        img       = f['imgName'].value;
        imgWidth  = f['imgWidth'].value;
        imgHeight = f['imgHeight'].value;
    }

    if (f['serendipity[filename_only]'] && f['serendipity[filename_only]'].value == 'true') {
        self.opener.serendipity_imageSelector_addToElement(img, f['serendipity[htmltarget]'].value);
        self.close();
        return true;
    }

    if (document.getElementById('serendipity_imagecomment').value != '') {
        styled = false;
    } else {
        styled = true;
    }

    floating = 'center';
    if (XHTML11) {
        if (f['serendipity[align]'][0].checked == true) {
            img = "<img width='" + imgWidth + "' height='" + imgHeight + "' " + (styled ? 'style="border: 0px; padding-left: 5px; padding-right: 5px;"' : '') + ' src="' + img + "\" alt=\"\" />";
        } else if (f['serendipity[align]'][1].checked == true) {
            img = "<img width='" + imgWidth + "' height='" + imgHeight + "' " + (styled ? 'style="float: left; border: 0px; padding-left: 5px; padding-right: 5px;"' : '') + ' src="' + img + "\" alt=\"\" />";
            floating = 'left';
        } else if (f['serendipity[align]'][2].checked == true) {
            img = "<img width='" + imgWidth + "' height='" + imgHeight + "' " + (styled ? 'style="float: right; border: 0px; padding-left: 5px; padding-right: 5px;"' : '') + ' src="' + img + "\" alt=\"\" />";
            floating = 'right';
        }
    } else {
        if (f['serendipity[align]'][0].checked == true) {
            img = "<img width='" + imgWidth + "' height='" + imgHeight + "' border='0' hspace='5' src='" + img + "' alt='' />";
        } else if (f['serendipity[align]'][1].checked == true) {
            img = "<img width='" + imgWidth + "' height='" + imgHeight + "' border='0' hspace='5' align='left' src='" + img + "' alt='' />";
            floating = 'left';
        } else if (f['serendipity[align]'][2].checked == true) {
            img = "<img width='" + imgWidth + "' height='" + imgHeight + "' border='0' hspace='5' align='right' src='" + img + "' alt='' />";
            floating = 'right';
        }
    }

    if (f['serendipity[isLink]'][1].checked == true) {
        insert = "<a href='" + f['serendipity[url]'].value + "'>" + img + "</a>";
    } else {
        insert = img;
    }

    if (document.getElementById('serendipity_imagecomment').value != '') {
        comment = f['serendipity[imagecomment]'].value;
        block = '<div class="serendipity_imageComment_' + floating + '" style="width: ' + imgWidth + 'px">'
              +     '<div class="serendipity_imageComment_img">' + insert + '</div>'
              +     '<div class="serendipity_imageComment_txt">' + comment + '</div>'
              + '</div>';
    } else {
        block = insert;
    }

    if (self.opener.editorref) {
        self.opener.editorref.surroundHTML(block, '');
    } else {
    	
        self.opener.serendipity_imageSelector_addToBody(block, textarea);
    }

    self.close();
}

// -->
