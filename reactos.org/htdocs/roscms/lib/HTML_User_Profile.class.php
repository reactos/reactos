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
   * search process
   *
   * @access private
   */
  protected function body( )
  {
    $config = &RosCMS::getInstance();
  
    if ($this->search && empty($_GET['user_id'])) {

      if (isset($_GET['search'])) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ".ROSCMST_USERS." WHERE name LIKE :nickname OR fullname LIKE :fullname");
        $stmt->bindValue('nickname','%'.$_GET['search'].'%',PDO::PARAM_STR);
        $stmt->bindValue('fullname','%'.$_GET['search'].'%',PDO::PARAM_STR);
        $stmt->execute();
        $users_found = $stmt->fetchColumn();

        if ($users_found == 1) {
          $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_USERS." WHERE name LIKE :nickname OR fullname LIKE :fullname LIMIT 1");
          $stmt->bindValue('nickname','%'.$_GET['search'].'%',PDO::PARAM_STR);
          $stmt->bindValue('fullname','%'.$_GET['search'].'%',PDO::PARAM_STR);
          $stmt->execute();
          $user_id = $stmt->fetchColumn();
        }
      } else {
        $users_found = false;
      }

      // more than one user was found (or none)
      if ($users_found != 1 && (empty($_GET['user_name']) || !empty($_GET['search']))) {
        echo_strip('
          <h1><a href="'.$config->pathRosCMS().'?page=my">'.$config->siteName().'</a> &gt; Profile Search</h1>
          <p>Profile Search</p>
          <form id="form1" method="get" action="'.$config->pathRosCMS().'?page=search">
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

          $stmt=&DBConnection::getInstance()->prepare("SELECT name, fullname, id FROM ".ROSCMST_USERS." WHERE name LIKE :nickname OR fullname LIKE :fullname ORDER BY name ASC LIMIT 100");
          $stmt->bindValue('nickname','%'.$_GET['search'].'%',PDO::PARAM_STR);
          $stmt->bindValue('fullname','%'.$_GET['search'].'%',PDO::PARAM_STR);
          $stmt->execute();

          while ($search = $stmt->fetch(PDO::FETCH_ASSOC)) {
            echo '<li><a style="font-weight:bold;" href="'.$config->pathRosCMS().'?page=search&amp;user_id='.$search['id'].'">'.$search['name'].'</a>';
            if ($search['fullname']) {
              echo '<br />'.$search['fullname'];
            }
            echo '<br />&nbsp;</li>';
          } // end while

          echo '</ul>&nbsp;';
        }
      }
      else {
        if (empty($user_id) || $user_id === false) {
          $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_USERS." WHERE name = :user_name LIMIT 1");
          $stmt->bindParam('user_name',rawurldecode(@$_GET['user_name']));
          $stmt->execute();
          $user_id = $stmt->fetchColumn();
        }

        // foreign profile
        $this->profile($user_id);
      }
    }

    // foreign profile
    elseif (isset($_GET['user_id']) && $_GET['user_id'] > 0) {
      $this->profile($_GET['user_id']);
    }

    // own profile
    else {
      $this->profile(ThisUser::getInstance()->id());
    }
  }

  /**
   *
   *
   * @access private
   */
  private function profile( $user_id )
  {
    $thisuser = &ThisUser::getInstance();
    $config = &RosCMS::getInstance();

    $stmt=&DBConnection::getInstance()->prepare("SELECT u.id, u.name, u.created, u.fullname, u.email, u.activation, u.homepage, c.name AS country, CONCAT(t.name,' (', t.difference,')') AS timezone, l.name AS language, u.occupation FROM ".ROSCMST_USERS." u LEFT JOIN ".ROSCMST_COUNTRIES." c ON u.country_id=c.id LEFT JOIN ".ROSCMST_TIMEZONES." t ON t.id=u.timezone_id LEFT JOIN ".ROSCMST_LANGUAGES." l ON l.id=u.lang_id WHERE u.id = :user_id LIMIT 1");
    $stmt->bindparam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute();
    $profile = $stmt->fetchOnce();

    // check if profile was found
    if ($profile === false) {
      echo 'This Profile does not exist!';
      return;
    }

    // begin output
    echo_strip('
      <h1>'.$config->siteName().' Profile</h1>
      <p>A person who joined '.$config->siteName().' on '.Date::getLocal($profile['created']).'.</p>

      <div class="bubble">
        <div class="corner_TL">
          <div class="corner_TR"></div>
        </div>
        <h2>'.$config->siteName().' Profile</h2>

        <div class="field">
          <div class="key">Username</div>
          <div class="value">'.htmlspecialchars($profile['name']).'</div>
        </div>');

    // Fullname
    if ($profile['fullname'] != '') {
      echo_strip('
        <div class="field">
          <div class="key">First and Last Name</div>
          <div class="value">'.htmlspecialchars($profile['fullname']).'</div>
        </div>');
    }

    // email only for the user itself or admins
    if ($profile['id'] == $thisuser->id() || $thisuser->securityLevel() == 3) {
      echo_strip('
        <div class="field">
          <div class="key">E-Mail Address </div>
          <div class="value">'.htmlspecialchars($profile['email']).'</div>
        </div>');
    }

    // Country and Language
    echo_strip('
      <div class="field">
        <div class="key">Country</div>
        <div class="value">'.(($profile['country'] != null) ? $profile['country'] : '<span style="color: red;">not set</span>').'</div>
      </div>

      <div class="field">
        <div class="key">Language</div>
        <div class="value">'.(($profile['language'] != null) ? $profile['language'] : '<span style="color: red;">not set</span>').'</div>
      </div>

      <div class="field">
        <div class="key">Timezone</div>
        <div class="value">'.(($profile['timezone'] != null) ? $profile['timezone'] : '<span style="color: red;">not set</span>').'</div>
      </div>');

    // Website
    if ($profile['homepage'] != '') {
      echo_strip('
        <div class="field">
          <div class="key">Private Website</div>
          <div class="value"><a href="'.$profile['homepage'].'" rel="nofollow">'.htmlspecialchars($profile['homepage']).'</a></div>
        </div>');
    }

    // Occupation
    if ($profile['occupation'] != '') {
      echo_strip('
        <div class="field">
          <div class="key">Occupation</div>
          <div class="value">'.htmlspecialchars($profile['occupation']).'</div>
        </div>');
    }

    // Groups (only for user itself) and admins
    if ($profile['id'] == $thisuser->id() || $thisuser->securityLevel() == 3) {
      echo_strip('
        <div class="field">
          <div class="key">User Groups</div>
            <ul class="value">');
      $stmt=&DBConnection::getInstance()->prepare("SELECT g.name FROM ".ROSCMST_GROUPS." g JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id WHERE m.user_id = :user_id ORDER BY g.name ASC");
      $stmt->bindparam('user_id',$profile['id'],PDO::PARAM_INT);
      $stmt->execute();
      while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
        echo '<li>'.$group['name'].'</li>';
      }

      echo_strip('
          </ul>
        </div>');
    }

    // Location
    echo_Strip('
        <div class="field">
          <a href="'.$config->pathGenerated().'peoplemap/">'.($profile['id']==$thisuser->id() ? 'My ' : '').'Location on the Map</a>
        </div>');

    // show edit or search link (depending if the current user is searched user)
    if ($profile['id'] == $thisuser->id()) {
      echo '<div class="u-link"><a href="'.$config->pathRosCMS().'?page=my&amp;subpage=edit">Edit My Profile</a></div>';
    }
    else {
      echo_strip('
          <div class="u-link">
            <a href="'.$config->pathRosCMS().'?page=search">raquo; Profile Search</a>
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
