<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>
                  2009  Danny Götte <dangerground@web.de>

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

    switch (action) {

      // search by
      case 'b': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="name">Account name</option><option value="rname">Real name</option><option value="email">E-Mail</option><option value="web">Homepage</option></select>';
        break;

      // columns
      case 'c': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="Real-Name">Real Name</option><option value="e-mail">E-Mail</option><option value="homepage">Homepage</option><option value="edits">Edits</option>';

        // apply language stuff
        if (!roscms_access['more_lang']) {
          filtentryselstrs2 += '<option value="lang">Language</option>';
        }

        // apply advanced filter
        if (roscms_access['more_filter']) {
          filtentryselstrs2 += '<option value="country">Country</option><option value="timezone">Timezone</option>';
        }

        // apply admin filter
        if (roscms_access['admin_filter']) {
          filtentryselstrs2 += '<option value="last-login">Last Login</option><option value="registered">Registered</option>'
        }

        filtentryselstrs2 += '</select>';
        break;

      // order by
      case 'o': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="asc">Ascending</option><option value="desc">Descending</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><option value="name">Account name</option><option value="Real-Name">Real name</option><option value="E-mail">E-Mail</option><option value="homepage">Homepage</option><option value="edits">Edits</option>';

        // apply language stuff
        if (!roscms_access['more_lang']) {
          filtentryselstrs2 += '<option value="language">Language</option>';
        }

        // apply advanced filter
        if (roscms_access['more_filter']) {
          filtentryselstrs2 += '<option value="country">Country</option><option value="timezone">Timezone</option>';
        }

        // apply admin filter
        if (roscms_access['admin_filter']) {
          filtentryselstrs2 += '<option value="last-login">Last Login</option><option value="registered">Registered</option>'
        }

        filtentryselstrs2 += '</select>';
        break;

      // group
      case 'g': 
        filtentryselstrs1 = '<select id="sfb'+filterid+'"><option value="is">is</option><option value="no">is not</option></select>';
        filtentryselstrs2 = '<select id="sfc'+filterid+'"><?php

// get groups
$stmt=&DBConnection::getInstance()->prepare("SELECT id, name, security_level FROM ".ROSCMST_GROUPS." ORDER BY name ASC");
$stmt->execute();
while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
  if ($thisuser->hasAccess('addlvl'.$group['security_level'].'group')) {
    echo '<option value="'.$group['id'].'">'.$group['name'].'</option>';
  }
}
?></select>';
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
  $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." WHERE id = :lang_id");
  $stmt->bindParam('lang_id',$thisuser->language(),PDO::PARAM_INT);
}
$stmt->execute();
while ($language = $stmt->fetch(PDO::FETCH_ASSOC)) {
  echo '<option value="'.$language['id'].'"'.(($language['id'] == $thisuser->language()) ? ' selected="selected"' : '').'>'.$language['name'].'</option>';
}

?></select>';
        break;
    } // end switch

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
