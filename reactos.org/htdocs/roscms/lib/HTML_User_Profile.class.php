<?php
    /*
    ReactOS DynamicFrontend (RDF)
    Copyright (C) 2008  Klemens Friedl <frik85@reactos.org>

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
 * class HTML_User_Profile
 * 
 */
class HTML_User_Profile extends HTML_User
{

  private $search;


  /**
   *
   *
   * @access public
   */
  public function __construct( $page_title = '', $search = false)
  {
    Login::required();
    $this->search = $search;
    parent::__construct( $page_title );
  }


  /**
   *
   *
   * @access private
   */
  protected function body( )
  {
    global $roscms_intern_page_link;

    if ($this->search) {

      if (isset($_GET['search'])) {
        $stmt=DBConnection::getInstance()->prepare("SELECT count(*) FROM users WHERE user_name LIKE :nickname OR user_fullname LIKE :fullname");
        $stmt->bindValue('nickname','%'.$_GET['search'].'%');
        $stmt->bindValue('fullname','%'.$_GET['search'].'%');
        $stmt->execute();
        $users_found = $stmt->fetchColumn();

        if ($users_found == 1) {
          $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM users WHERE user_name LIKE :nickname OR user_fullname LIKE :fullname LIMIT 1");
          $stmt->bindValue('nickname','%'.$_GET['search'].'%');
          $stmt->bindValue('fullname','%'.$_GET['search'].'%');
          $stmt->execute();
          $user_id = $stmt->fetchColumn();
        }
      } else {
        $users_found = -1;
      }

      // more than one user was found (or none)
      if ($users_found != 1 && (empty($_GET['user_name']) || !empty($_GET['search']))) {
        echo_strip('
          <h1><a href="'.$roscms_intern_page_link.'my">myReactOS</a> &gt; Profile Search</h1>
          <p>Profile Search</p>
          <form id="form1" method="get" action="'.$roscms_intern_page_link.'search">
            <div class="bubble">
              <div class="corner_TL">
                <div class="corner_TR"></div>
              </div>
              <h2>Profile Search</h2>
              <div class="field">
                <label for="search">Username</label>
                <input name="search" type="text" id="search" value="'.@htmlentities($_GET['search']).'" />
              </div>
              <div class="field">
                <input type="hidden" name="page" id="page" value="search" />
                <button type="submit">Search</button>
              </div>
              <div class="corner_BL">
                <div class="corner_BR"></div>
              </div>
            </div>
          </form>');

        // give back results
        if (isset($_GET['search']) && $_GET['search'] != '') {
          echo '<ul>';

          $stmt=DBConnection::getInstance()->prepare("SELECT user_name, user_fullname FROM users WHERE user_name LIKE :nickname OR user_fullname LIKE :fullname ORDER BY user_name ASC LIMIT 100");
          $stmt->bindValue('nickname','%'.$_GET['search'].'%');
          $stmt->bindValue('fullname','%'.$_GET['search'].'%');
          $stmt->execute();

          while ($search = $stmt->fetch(PDO::FETCH_ASSOC)) {
            echo '<li><a style="font-weight:bold;" href="'.$roscms_intern_page_link.'search&amp;phrase'.$search['user_name'].'">'.$search['user_name'].'</a>';
            if ($search['user_fullname']) {
              echo '<br />'.$search['user_fullname'];
            }
            echo '<br />&nbsp;</li>';
          } // end while

          echo '</ul>&nbsp;';
        }
      }
      else {
        if (empty($user_id)|| $user_id === false) {
          $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM users WHERE user_name = :user_name LIMIT 1");
          $stmt->bindParam('user_name',rawurldecode(@$_GET['user_name']));
          $stmt->execute();
          $user_id = $stmt->fetchColumn();
        }
        $this->profile($user_id);
      }
    }
    else {
      $this->profile(ThisUser::getInstance()->id());
    }
  }

  /**
   *
   *
   * @access private
   */
  private function profile( $user_id = null )
  {
    global $roscms_intern_page_link;
    global $roscms_intern_webserver_pages;
    global $rdf_name;

    $thisuser = &ThisUser::getInstance();

    $stmt=DBConnection::getInstance()->prepare("SELECT user_id, user_name, user_register, user_fullname, user_email, user_email_activation, user_website, user_country, user_timezone, user_occupation, user_setting_multisession, user_setting_browseragent, user_setting_ipaddress, user_setting_timeout, user_language FROM users WHERE user_id = :user_id LIMIT 1");
    $stmt->bindparam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute();
    $profile = $stmt->fetchOnce();

    // check if profile was found
    if ($profile === false) {
      echo 'This Profile does not exist!';
      return;
    }

    // prepare
    $country = ROSuser::getCountry($profile['user_id']);
    $language = ROSuser::getLanguage($profile['user_id']);

    // begin output
    echo_strip('
      <h1>myReactOS Profile</h1>
      <p>A person who joined '.$rdf_name.' on '.Date::getLocal($profile['user_register']).'.</p>

      <div class="bubble">
        <div class="corner_TL">
          <div class="corner_TR"></div>
        </div>
        <h2>myReactOS Profile</h2>

        <div class="field">
          <div class="key">Username</div>
          <div class="value">'.htmlspecialchars($profile['user_name']).'</div>
        </div>');

    // Fullname
    if ($profile['user_fullname'] != '') {
      echo_strip('
        <div class="field">
          <div class="key">First and Last Name</div>
          <div class="value">'.htmlspecialchars($profile['user_fullname']).'</div>
        </div>');
    }

    // email only for the user itself or admins
    if ($profile['user_id'] == $thisuser->id() || $thisuser->securityLevel() == 3) {
      echo_strip('
        <div class="field">
          <div class="key">E-Mail Address </div>
          <div class="value">'.htmlspecialchars($profile['user_email']).'</div>
        </div>');
    }

    // Country and Language
    echo_strip('
      <div class="field">
        <div class="key">Country</div>
        <div class="value">'.(($country !== false) ? $country : '<span style="color: red;">not set</span>').'</div>
      </div>

      <div class="field">
        <div class="key">Language</div>
        <div class="value">'.(($language !== false) ? $language : '<span style="color: red;">not set</span>').'</div>
      </div>

      <div class="field">
        <div class="key">Timezone</div>
        <div class="value">');

    // Timezone
    if (ROSUser::checkTimezone($profile['user_timezone'])) {
      $stmt=DBConnection::getInstance()->prepare("SELECT tz_code, tz_name, tz_value2 FROM user_timezone WHERE tz_code = :tz_code LIMIT 1");
      $stmt->bindparam('tz_code',$profile['user_timezone'],PDO::PARAM_STR);
      $stmt->execute();
      $timezone = $stmt->fetchOnce();

      echo_strip(
        $timezone['tz_name'].' ('.$timezone['tz_value2'].')
        <div class="detail">
          server time: '.date('Y-m-d H:i').'<br />
          local time: '.Date::getLocal(date('Y-m-d H:i')).'
        </div>');
    }
    else {
      echo '<span style="color: red;">not set</span>';
    }
    echo_strip('
        </div>
      </div>');

    // Website
    if ($profile['user_website'] != '') {
      echo_strip('
        <div class="field">
          <div class="key">Private Website</div>
          <div class="value"><a href="'.$profile['user_website'].'" rel="nofollow">'.htmlspecialchars($profile['user_website']).'</a></div>
        </div>');
    }

    // Occupation
    if ($profile['user_occupation'] != '') {
      echo_strip('
        <div class="field">
          <div class="key">Occupation</div>
          <div class="value">'.htmlspecialchars($profile['user_occupation']).'</div>
        </div>');
    }

    // Groups (only for user itself) and admins
    if ($profile['user_id'] == $thisuser->id() || $thisuser->securityLevel() == 3) {
      echo_strip('
        <div class="field">
          <div class="key">User Groups</div>
            <ul class="value">');
      $stmt=DBConnection::getInstance()->prepare("SELECT u.usrgroup_name FROM usergroups u, usergroup_members m WHERE m.usergroupmember_userid = :user_id AND u.usrgroup_name_id = m.usergroupmember_usergroupid ORDER BY usrgroup_securitylevel DESC, usrgroup_name ASC");
      $stmt->bindparam('user_id',$profile['user_id'],PDO::PARAM_INT);
      $stmt->execute();
      while ($usergroup = $stmt->fetch()) {
        echo '<li>'.$usergroup['usrgroup_name'].'</li>';
      }

      echo_strip('
          </ul>
        </div>');
    }

    // Location
    echo_Strip('
        <div class="field">
          <a href="'.$roscms_intern_webserver_pages.'peoplemap/">'.($profile['user_id']==$thisuser->id() ? 'My ' : '').'Location on the Map</a>
        </div>');

    // show edit or search link (depending if the current user is searched user)
    if ($profile['user_id'] == $thisuser->id()) {
      echo '<div class="u-link"><a href="'.$roscms_intern_page_link.'my&amp;subpage=edit">Edit My Profile</a></div>';
    }
    else {
      echo_strip('
          <div class="u-link">
            <a href="'.$roscms_intern_page_link.'search">raquo; Profile Search</a>
          </div>');
    }
      echo_strip('
          <div class="corner_BL">
            <div class="corner_BR"></div>
          </div>
        </div>');
  }



} // end of HTML_User_Profile
?>
