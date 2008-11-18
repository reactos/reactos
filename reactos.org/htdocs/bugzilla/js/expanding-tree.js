/* The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Bugzilla Bug Tracking System.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): Mike Shaver <shaver@mozilla.org>
 *                 Christian Reis <kiko@async.com.br>
 *                 André Batosti <batosti@async.com.br> 
 */

if (!Node) {
    // MSIE doesn't define Node, so provide a compatibility object
    var Node = { TEXT_NODE: 3 }
}

if (!highlighted) {
    var highlighted = 0;
    var highlightedclass = "";
    var highlightedover = 0;
}

function doToggle(node, event) {
    var deep = event.altKey || event.ctrlKey;

    if (node.nodeType == Node.TEXT_NODE)
        node = node.parentNode;

    var toggle = node.nextSibling;
    while (toggle && toggle.tagName != "UL")
        toggle = toggle.nextSibling;

    if (toggle) {
        if (deep) {
            var direction = toggleDisplay(toggle, node);
            changeChildren(toggle, direction);
        } else {
            toggleDisplay(toggle, node);
        }
    }
    /* avoid problems with default actions on links (mozilla's
     * ctrl/shift-click defaults, for instance */
    event.preventBubble();
    event.preventDefault();
    return false;
}

function changeChildren(node, direction) {
    var item = node.firstChild;
    while (item) {
        /* find the LI inside the UL I got */
        while (item && item.tagName != "LI")
            item = item.nextSibling;
        if (!item)
            return;

        /* got it, now find the first A */
        var child = item.firstChild;
        while (child && child.tagName != "A")
            child = child.nextSibling;
        if (!child) {
            return
        }
        var bullet = child;

        /* and check if it has its own sublist */
        var sublist = item.firstChild;
        while (sublist && sublist.tagName != "UL")
            sublist = sublist.nextSibling;
        if (sublist) {
            if (direction && isClosed(sublist)) {
                openNode(sublist, bullet);
            } else if (!direction && !isClosed(sublist)) {
                closeNode(sublist, bullet);
            }
            changeChildren(sublist, direction)
        }
        item = item.nextSibling;
    }
}

function openNode(node, bullet) {
    node.style.display = "block";
    bullet.className = "b b_open";
}

function closeNode(node, bullet) {
    node.style.display = "none";
    bullet.className = "b b_closed";
}

function isClosed(node) {
    /* XXX we should in fact check our *computed* style, not the display
     * attribute of the current node, which may be inherited and not
     * set. However, this really only matters when changing the default
     * appearance of the tree through a parent style. */
    return node.style.display == "none";
}

function toggleDisplay(node, bullet) {
    if (isClosed(node)) {
        openNode(node, bullet);
        return true;
    }

    closeNode(node, bullet);
    return false;
}

function duplicated(element) {
    var allsumm= document.getElementsByTagName("span");
    if (highlighted) {
        for (i = 0;i < allsumm.length; i++) {
            if (allsumm.item(i).id == highlighted) {
                allsumm.item(i).className = highlightedclass;
            }
        }
        if (highlighted == element) {
            highlighted = 0;
            return;
        }
    } 
    highlighted = element;
    var elem = document.getElementById(element);
    highlightedclass = elem.className;
    for (var i = 0;i < allsumm.length; i++) {
        if (allsumm.item(i).id == element) {
            allsumm.item(i).className = "summ_h";
        }
    }
}

function duplicatedover(element) {
    if (!highlighted) {
        highlightedover = 1;
        duplicated(element);
    }
}

function duplicatedout(element) {
    if (highlighted == element && highlightedover) {
        highlightedover = 0;
        duplicated(element);
    }
}

