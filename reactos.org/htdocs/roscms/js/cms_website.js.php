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

// config data
require_once(ROSCMS_PATH.'config.php');
RosCMS::getInstance()->apply();
Login::required();

// get user language
$thisuser = ThisUser::getInstance();
?>


/**
 * gives possible values for a selected filter
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
  if (objidval2 == '0' && !roscms_access.dont_hide_filter) { 
    filtentryselstrs1 = '<input type="hidden" name="sfb'+filterid+'" id="sfb'+filterid+'" value="" />';
    filtentryselstrs2 = '<input type="hidden" name="sfc'+filterid+'" id="sfc'+filterid+'" value="" />';
  }
  else {

    switch (action) {

      // kind
      case 'k': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access['more_filter']) {
          filtentryselstrs1 += '<option value="no">is not</option>';
        }
        filtentryselstrs1 += '</select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="stable">Published</option><option value="new">Pending</option><option value="draft">Draft</option><option value="unknown">Unknown or no status</option>';

        if (roscms_access['more_filter']) {
          filtentryselstrs2 += '<option value="archive">Archive</option>';
        }

        filtentryselstrs2 += '</select>';
        break;

      // type
      case 'y': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access['more_filter']) {
          filtentryselstrs1 += '<option value="no">is not</option>';
        }
        filtentryselstrs1 += '</select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="page">Page</option><option value="dynamic">Dynamic Page</option><option value="content">Content</option><option value="script">Script</option><option value="system">System</option></select>';
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

      // language
      case 'l': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php

// build languages
if ($thisuser->hasAccess('more_lang')) {
  $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE level > 0 ORDER BY name ASC");
}
else {
  $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE id IN(:lang_id,:standard_lang)");
  $stmt->bindParam('lang_id',$thisuser->language(),PDO::PARAM_INT);
  $stmt->bindParam('standard_lang',Language::getStandardId(),PDO::PARAM_INT);
}
$stmt->execute();
while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {
  echo '<option value="'.$language['id'].'"'.(($language['id'] == $thisuser->language()) ? ' selected="selected"' : '').'>'.$language['name'].'</option>';
}

?></select>';
        break;

      // translate
      case 'r': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">to</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php

// translation languages
if ($thisuser->hasAccess('more_lang')) {
  $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE level > 0 AND id!=:standard_lang ORDER BY name ASC");
}
else {
  $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE id=:lang_id AND id != :standard_lang");
  $stmt->bindParam('lang_id',$thisuser->language(),PDO::PARAM_INT);
}
$stmt->bindParam('standard_lang',Language::getStandardId(),PDO::PARAM_INT);
$stmt->execute();
while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {
  echo '<option value="'.$language['id'].'"'.(($language['id'] == $thisuser->language()) ? ' selected="selected"' : '').'>'.$language['name'].'</option>';
}
 ?></select>';
        break;

      // user
      case 'u': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access['more_filter']) {
          filtentryselstrs1 += '<option value="no">is not</option>';
        }
        filtentryselstrs1 += '</select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. John Doe)';
        break;

      // column
      case 'c':
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="language">Language</option><option value="user">User</option><option value="type">Type</option><option value="version">Version</option><option value="security">Security</option><option value="date">Date</option><option value="title">Title</option></select>';
        break;

      // order by
      case 'o': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="asc">Ascending</option><option value="desc">Descending</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="date">Date &amp; Time</option><option value="name">Name</option><option value="language">Language</option><option value="user">User</option><option value="type">Type</option><option value="version">Version</option><option value="number">Number ("dynamic" entry)</option>';
        if (roscms_access['more_filter']) {
          filtentryselstrs2 += '<option value="security">Security</option><option value="revid">RevID</option><option value="ext">Extension</option><option value="status">Status</option><option value="kind">Kind</option>';
        }
        filtentryselstrs2 += '</select>';
        break;

      // security (ACL)
      case 'i': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php

// ACL
$stmt=&DBConnection::getInstance()->prepare("SELECT id, name, name_short FROM ".ROSCMST_RIGHTS." ORDER BY name ASC");
$stmt->execute();
while($ACL=$stmt->fetch(PDO::FETCH_ASSOC)) {
  echo '<option value="'. $ACL['name_short'] .'">'. $ACL['name'] .'</option>';
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
        if (roscms_access['more_filter']) {
          filtentryselstrs1 += '<option value="no">is not</option><option value="likea">is like *...*</option>';
        }
        filtentryselstrs1 += '<option value="likeb">is like ...*</option></select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="20" maxlength="50" />&nbsp;&nbsp;(e.g. about)';
        break;

      // tag
      case 'a': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option>';
        if (roscms_access['more_filter']) {
          filtentryselstrs1 += '<option value="no">is not</option>';
        }
        filtentryselstrs1 +='</select>';
        filtentryselstrs2 = '<input id="sfc'+filterid+'" type="text" value="" size="15" maxlength="30" />&nbsp;&nbsp;(e.g. todo)';
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
