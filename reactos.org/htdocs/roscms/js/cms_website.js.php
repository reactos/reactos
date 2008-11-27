<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

define('ROSCMS_PATH', '../');
require('../lib/RosCMS_Autoloader.class.php');
Login::required();

// get user language
$user_lang = ROSUser::getLanguage(ThisUser::getInstance()->id(), true);

// prepare build languages
$stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_level, lang_name FROM languages WHERE lang_level > '0' ORDER BY lang_name ASC");
$stmt->execute();
$languages = $stmt->fetchAll(PDO::FETCH_ASSOC);
?>


/**
 * @FILLME
 *
 * @param string action see this switch statement for more
 * @param int objidval2
 * @param int filterid
 *
 */
function htmlFilterValues( action, objidval2, filterid ) {
  var filtentryselstr = '';
  var filtentryselstrs1 = '';
  var filtentryselstrs2 = '';

  // hidden filter entries don't need a combobox (only for SecLev = 1 user) 
  if (objidval2 == 0 && roscms_access_level == 1) { 
    filtentryselstrs1 = '<input type="hidden" name="sfb'+filterid+'" id="sfb'+filterid+'" value="" />';
    filtentryselstrs2 = '<input type="hidden" name="sfc'+filterid+'" id="sfc'+filterid+'" value="" />';
  }
  else {

    switch (action) {

      // kind
      case 'k': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access_level > 1) {
          filtentryselstrs1 += '<option value="no"'+roscms_cbm_hide+'>is not</option>';
        }
        filtentryselstrs1 += '</select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="stable">Stable</option><option value="new">New</option><option value="draft">Draft</option><option value="unknown">Unknown or no status</option>';

        if (roscms_access_level > 1) {
          filtentryselstrs2 += '<option value="archive"+roscms_cbm_hide+>Archive</option>';
        }

        filtentryselstrs2 += '</select>';
        break;

      // type
      case 'y': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access_level > 1) {
          filtentryselstrs1 += '<option value="no"+roscms_cbm_hide+>is not</option>';
        }
        filtentryselstrs1 += '</select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="page">Page</option><option value="content">Content</option><option value="template">Template</option><option value="script">Script</option><option value="system">System</option></select>';
        break;

      // starred
      case 's': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="true">on</option><option value="false">off</option></select>';
        break;

      // date
      case 'd': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. 2007-02-22)';
        break;

      // time
      case 't': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. 15:30)';
        break;

      // language
      case 'l': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php

// build languages
foreach($languages as $language) {
  echo '<option value="'.$language['lang_id'].'"'.(($language['lang_id'] == $user_lang) ? ' selected="selected"' : '').'>'.$language['lang_name'].'</option>';
}

?></select>';
        break;

      // translate
      case 'r': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">to</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php

// build translation languages
foreach($languages as $language) {
  if ($language['lang_level'] != '10') {
    echo '<option value="'.$language['lang_id'].'"'.(($language['lang_id'] == $user_lang) ? ' selected="selected"' : '').'>'.$language['lang_name'].'</option>';
  }
}
 ?></select>';
        break;

      // user
      case 'u': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access_level > 1) {
          filtentryselstrs1 += '<option value="no"+roscms_cbm_hide+>is not</option>';
        }
        filtentryselstrs1 += '</select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. John Doe)';
        break;

      // version
      case 'v': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option><option value="sm">is smaller</option><option value="la">is larger</option></select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="5" maxlength="10" />&nbsp;&nbsp;(e.g. 12)';
        break;

      // column
      case 'c':
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="language">Language</option><option value="user">User</option><option value="type">Type</option><option value="version">Version</option>';
        if (roscms_access_level > 1) {
          filtentryselstrs2 += '<option value="security"+roscms_cbm_hide+>Security</option><option value="rights"+roscms_cbm_hide+>Rights</option>';
        }
        filtentryselstrs2 += '</select>';
        break;

      // order by
      case 'o': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="asc">Ascending</option><option value="desc">Descending</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="datetime">Date &amp; Time</option><option value="name">Name</option><option value="lang">Language</option><option value="usr">User</option><option value="type">Type</option><option value="ver">Version</option><option value="nbr">Number ("dynamic" entry)</option>';
        if (roscms_access_level > 1) {
          filtentryselstrs2 += '<option value="security"+roscms_cbm_hide+>Security</option><option value="revid"+roscms_cbm_hide+>RevID</option><option value="ext"+roscms_cbm_hide+>Extension</option><option value="status"+roscms_cbm_hide+>Status</option><option value="kind"+roscms_cbm_hide+>Kind</option>';
        }
        filtentryselstrs2 += '</select>';
        break;

      // security (ACL)
      case 'i': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php

// ACL
$stmt=DBConnection::getInstance()->prepare("SELECT sec_name, sec_fullname FROM data_security ORDER BY sec_fullname ASC");
$stmt->execute();
while($ACL=$stmt->fetch(PDO::FETCH_ASSOC)) {
  echo '<option value="'. $ACL['sec_name'] .'">'. $ACL['sec_fullname'] .'</option>';
}

 ?></select>';
        break;

      // metadata
      case 'm': 
        filtentryselstrs1 = '<input id="sfb'+filterid+'" type="text" value="" size="10" maxlength="50" />';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(entry: value)';
        break;

      // name
      case 'n': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access_level > 1) {
          filtentryselstrs1 += '<option value="no"+roscms_cbm_hide+>is not</option><option value="likea"+roscms_cbm_hide+>is like *...*</option>';
        }
        filtentryselstrs1 += '<option value="likeb">is like ...*</option></select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. about)';
        break;

      // tag
      case 'a': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access_level > 1) {
          filtentryselstrs1 += '<option value="no"+roscms_cbm_hide+>is not</option>';
        }
        filtentryselstrs1 +='</select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="15" maxlength="30" />&nbsp;&nbsp;(e.g. todo)';
        break;

      // system
      case 'e': 
        if (roscms_access_level == 3) {
          filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="dataid">Data-ID</option><option value="revid">Rev-ID</option><option value="usrid">User-ID</option><option value="langid">Lang-ID</option></select>';
          filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="15" maxlength="30" />';
        }
        break;
    } // end switch
  }

  filtentryselstr += '<span id="sfz'+filterid+'">'
    + filtentryselstrs1
    + '</span>&nbsp;'
    + '<span id="sfy'+filterid+'">'
    + filtentryselstrs2
    + '</span>'
    + '<span id="sfx'+filterid+'">'
    + '<input name="sfd'+filterid+'" type="hidden" value="'+objidval2+'" />'
    + '</span>';

  return filtentryselstr;
}
