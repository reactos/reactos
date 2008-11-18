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
 * Contributor(s): Myk Melez <myk@mozilla.org>
 *                 Joel Peshkin <bugreport@peshkin.net>
 *                 Erik Stambaugh <erik@dasbistro.com>
 *                 Marc Schumann <wurblzap@gmail.com>
 */

function updateCommentPrivacy(checkbox) {
    var text_elem = document.getElementById('comment');
    if (checkbox.checked) {
        text_elem.className='bz_private';
    } else {
        text_elem.className='';
    }
}

function setContentTypeDisabledState(form)
{
    var isdisabled = false;
    if (form.ispatch.checked)
        isdisabled = true;

    for (var i=0 ; i<form.contenttypemethod.length ; i++)
        form.contenttypemethod[i].disabled = isdisabled;

    form.contenttypeselection.disabled = isdisabled;
    form.contenttypeentry.disabled = isdisabled;
}

function URLFieldHandler() {
    var field_attachurl = document.getElementById("attachurl");
    var greyfields = new Array("data", "ispatch", "autodetect",
                               "list", "manual", "bigfile",
                               "contenttypeselection",
                               "contenttypeentry");
    var i, thisfield;
    if (field_attachurl.value.match(/^\s*$/)) {
        for (i = 0; i < greyfields.length; i++) {
            thisfield = document.getElementById(greyfields[i]);
            if (thisfield) {
                thisfield.removeAttribute("disabled");
            }
        }
    } else {
        for (i = 0; i < greyfields.length; i++) {
            thisfield = document.getElementById(greyfields[i]);
            if (thisfield) {
                thisfield.setAttribute("disabled", "disabled");
            }
        }
    }
}

function DataFieldHandler() {
    var field_data = document.getElementById("data");
    var greyfields = new Array("attachurl");
    var i, thisfield;
    if (field_data.value.match(/^\s*$/)) {
        for (i = 0; i < greyfields.length; i++) {
            thisfield = document.getElementById(greyfields[i]);
            if (thisfield) {
                thisfield.removeAttribute("disabled");
            }
        }
    } else {
        for (i = 0; i < greyfields.length; i++) {
            thisfield = document.getElementById(greyfields[i]);
            if (thisfield) {
                thisfield.setAttribute("disabled", "disabled");
            }
        }
    }
}

function clearAttachmentFields() {
    var element;

    document.getElementById('data').value = '';
    DataFieldHandler();
    if ((element = document.getElementById('bigfile')))
        element.checked = '';
    if ((element = document.getElementById('attachurl'))) {
        element.value = '';
        URLFieldHandler();
    }
    document.getElementById('description').value = '';
    document.getElementById('ispatch').checked = '';
    if ((element = document.getElementById('isprivate')))
        element.checked = '';
}
