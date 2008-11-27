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
    global $roscms_standard_language;

    $thisuser = &ThisUser::getInstance();

    if (!$thisuser->isMemberOfGroup('transmaint','ros_admin','ros_sadmin')) {
      return;
    }

    echo_strip('
      <br />
      <h2>User</h2>
      <p style="font-weight: bold;">User Account Management Interface</p>
      <br />');
      
    if ($thisuser->isMemberOfGroup('ros_admin','ros_sadmin')) {
      echo '<h3>Administrator</h3>';
    }
    elseif ($thisuser->isMemberOfGroup('transmaint')) {
      echo '<h3>Language Maintainer</h3>';
      $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, u.user_id, u.user_name, u.user_fullname, u.user_language, COUNT(r.data_id) as 'editcounter' FROM data_a d, data_revision r, users u WHERE r.data_id = d.data_id AND r.rev_usrid = u.user_id AND rev_version  > 0  AND r.rev_language = :lang GROUP BY u.user_name ORDER BY editcounter DESC, u.user_name");
      $stmt->bindParam('lang',$roscms_standard_language,PDO::PARAM_STR);
    }

    echo_strip('
      <div>
        <label for="textfield">Username: </label><input type="text" name="textfield" id="textfield" onkeyup="getUser()" /><br />
        <input name="searchopt" type="radio" id="searchopt1" value="accountname" checked="checked" onclick="getUser()" /><label>account name</label>
        <input name="searchopt" type="radio" id="searchopt2" value="fullname" onclick="getUser()" /><label>full name </label>
        <input name="searchopt" type="radio" id="searchopt3" value="email" onclick="getUser()" /><label>email address</label>
        <input name="searchopt" type="radio" id="searchopt4" value="website" onclick="getUser()" /><label>website</label>
        <input name="searchopt" type="radio" id="searchopt5" value="language" onclick="getUser()" /><label>language</label>
        <img id="ajaxloading" style="display:none;" src="images/ajax_loading.gif" width="13" height="13" alt="" /><br />
        <br />
      </div>
      <div id="userarea"></div>
      <br />
      <br />
      <h4>Translators</h4>
      <ul>');

    if (empty($stmt)) {
      $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, u.user_id, u.user_name, u.user_fullname, u.user_language, COUNT(r.data_id) as 'editcounter' FROM data_a d, data_revision r, users u WHERE r.data_id = d.data_id AND r.rev_usrid = u.user_id AND rev_version  > 0 GROUP BY u.user_name ORDER BY editcounter DESC, u.user_name");
    }
     $stmt->execute();
    while ($translator = $stmt->fetch()) {
      echo '<li>'.$translator['user_name'].' ('.$translator['user_fullname'].'; '.$translator['user_language'].') '.$translator['editcounter'].' stable edits</li>';
    }

    echo_strip('
      </ul>
      <br />');
  }


} // end of HTML_CMS_User
?>
