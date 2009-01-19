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


/**
 * class HTML_CMS_User
 * 
 */
class HTML_CMS_User extends HTML_CMS
{


  /**
   *
   *
   * @access public
   */
  public function __construct( $page_title = '', $page_css = 'roscms' )
  {
    $this->branch = 'user';

    // register css & js files
    $this->register_css('cms_user.css');
    $this->register_js('cms_user.js');

    parent::__construct( $page_title, $page_css);
  }


  /**
   *
   *
   * @access protected
   */
  protected function body( )
  {
    $thisuser = &ThisUser::getInstance();

    if (!$thisuser->hasAccess('user')) {
      return;
    }

    echo_strip('
      <br />
      <h2>User</h2>
      <p style="font-weight: bold;">User Account Management Interface</p>
      <br />');

    echo_strip('
      <div>
        <label for="textfield">Username: </label><input type="text" name="textfield" id="textfield" onkeyup="getUser()" /><br />
        <input name="searchopt" type="radio" id="searchopt1" value="accountname" checked="checked" onclick="getUser()" /><label>account name</label>
        <input name="searchopt" type="radio" id="searchopt2" value="fullname" onclick="getUser()" /><label>full name </label>
        <input name="searchopt" type="radio" id="searchopt3" value="email" onclick="getUser()" /><label>email address</label>
        <input name="searchopt" type="radio" id="searchopt4" value="website" onclick="getUser()" /><label>website</label>'.($thisuser->hasAccess('more_lang') ? '
        <input name="searchopt" type="radio" id="searchopt5" value="language" onclick="getUser()" /><label>language</label>' : '').'
        <img id="ajaxloading" style="display:none;" src="images/ajax_loading.gif" width="13" height="13" alt="" /><br />
        <br />
      </div>
      <div id="userarea"></div>
      <br />
      <br />
      <h4>Translators</h4>');
    // get list of translators
    if ($thisuser->hasAccess('more_lang')) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT u.id, u.name, u.fullname, l.name AS language, COUNT(r.id) as editcounter FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_USERS." u ON r.user_id = u.id JOIN ".ROSCMST_LANGUAGES." l ON u.lang_id=l.id WHERE r.version > 0 GROUP BY u.id ORDER BY l.level DESC, l.name ASC, editcounter DESC, u.name ASC");
    }

    // get list only for one language
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT u.id, u.name, u.fullname, l.name AS language, (SELECT COUNT(id) FROM ".ROSCMST_REVISIONS." WHERE user_id = u.id AND version > 0) as editcounter FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_LANGUAGES." l ON u.lang_id=l.id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.user_id=u.id JOIN ".ROSCMST_GROUPS." g ON g.id=m.group_id WHERE g.name_short='translator' AND u.lang_id = :lang ORDER BY editcounter DESC, u.name ASC");
      $stmt->bindParam('lang',ROSUser::getLanguage($thisuser->id(),true),PDO::PARAM_INT);
    }

    // output list of translators
    $stmt->execute();
    $oldlang = null;
    while ($translator = $stmt->fetch(PDO::FETCH_ASSOC)) {
      if ($oldlang != $translator['language']) {
        if ($oldlang !== null) {
          echo '</ul>';
        }
        echo '<h5>'.$translator['language'].'</h5><ul>';
      }
      $oldlang = $translator['language'];
      echo '<li>'.$translator['name'].' ('.$translator['fullname'].') '.$translator['editcounter'].' stable edits</li>';
    }

    echo_strip('
      </ul>
      <br />');
  }


} // end of HTML_CMS_User
?>
