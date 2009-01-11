<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Ge van Geldorp <gvg@reactos.org>
                  2005  Klemens Friedl <frik85@reactos.org>

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
 * class Login
 * 
 */
class Login
{
  const OPTIONAL = 1;
  const REQUIRED = 2;


  /**
   *
   *
   * @param string target to jump back after login process
   * @param string subsystem name which is called
   * @return int
   * @access public
   */
  public static function in( $login_type, $target, $subsys = '' )
  {
    // we need this include, as the method can also be called through subsystems
    require_once(ROSCMS_PATH.'config.php');
    $user_id = 0;
    $config = &RosCMS::getInstance();

    if ( $login_type != self::OPTIONAL && $login_type != self::REQUIRED ){
      die('Invalid login_type '.$login_type.' for roscms_subsys_login');
    }

    // do update work, if a session is started
    if (isset($_COOKIE[$config->cookieUserKey()]) && preg_match('/^([a-z]{32})$/', $_COOKIE[$config->cookieUserKey()], $matches)) {
      $session_id_clean = $matches[1];
      // get a valid ip
      if (isset($_SERVER['REMOTE_ADDR']) && preg_match('/^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})$/', $_SERVER['REMOTE_ADDR'], $matches) ) {
          $remote_addr_clean = $matches[1];
      }
      else{
          $remote_addr_clean = 'invalid';
      }

      // get user agent
      if (isset($_SERVER['HTTP_USER_AGENT'])) {
          $browser_agent_clean = $_SERVER['HTTP_USER_AGENT'];
      }
      else {
          $browser_agent_clean = 'unknown';
      }

      // Clean out expired sessions
      DBConnection::getInstance()->exec("DELETE FROM ".ROSCMST_SESSIONS." WHERE expires IS NOT NULL AND expires < NOW()");

      // Now, see if we have a valid login session
      if ($subsys == '') {
        $stmt=&DBConnection::getInstance()->prepare("SELECT s.user_id, s.expires FROM ".ROSCMST_SESSIONS." s JOIN ".ROSCMST_USERS." u ON u.id = s.user_id WHERE s.id = :session_id AND (u.match_ip IS FALSE OR s.ip=:ip ) AND (u.match_browseragent IS FALSE OR s.browseragent = :agent) AND u.disabled IS FALSE LIMIT 1");
      }
      else{
        $stmt=&DBConnection::getInstance()->prepare("SELECT m.user_id, s.expires FROM ".ROSCMST_SESSIONS." s JOIN ".ROSCMST_USERS." u ON u.id = s.user_id JOIN ".ROSCMST_SUBSYS." m ON m.user_id = s.user_id WHERE s.id = :session_id AND (u.match_ip IS FALSE OR s.ip = :ip) AND (u.match_browseragent IS FALSE OR s.browseragent = :agent) AND m.subsys = :subsys AND u.disabled IS FALSE LIMIT 1");
          $stmt->bindParam('subsys',$subsys,PDO::PARAM_STR);
      }
      $stmt->bindParam('session_id',$session_id_clean,PDO::PARAM_INT);
      $stmt->bindParam('ip',$remote_addr_clean,PDO::PARAM_STR);
      $stmt->bindParam('agent',$browser_agent_clean,PDO::PARAM_STR);
      $stmt->execute() or die('DB error (user login) ');

      if ($row = $stmt->fetchOnce(PDO::FETCH_ASSOC)) {
        // Login session found
        $user_id = $row['user_id'];
        
        // Session with timeout. Update the expiry time in the table and the expiry time of the cookie
        if (isset($row['expires'])){
          $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_SESSIONS." SET expires = DATE_ADD(NOW(), INTERVAL 30 MINUTE) WHERE id = :session_id");
          $stmt->bindParam('session_id',$session_id_clean,PDO::PARAM_INT);
          $stmt->execute();
          setcookie($config->cookieUserKey(), $session_id_clean, time() + 30 * 60, '/', Cookie::getDomain());
        }
      }
    } // session check

    // goto login page, if login is required and no valid session was found
    if (0 == $user_id && $login_type == self::REQUIRED) {
      $url = $config->pathRosCMS().'?page=login';
      if ($target != '') {
        $url .= '&target='.urlencode($target);
      }

      header('Location: '.$url);
      exit;
    }

    return $user_id;
  } // end of member function login


  /**
   *
   *
   * @param string target to jump back after login process
   * @param string subsystem name which is called
   * @return int
   * @access public
   */
  public static function out( $target = '' )
  {
    $user_id = ThisUser::getInstance()->id();
    $config = &RosCMS::getInstance();

    if ($_COOKIE[$config->cookieUserKey()]) {
      // delete cookie
      $del_session_id = $_COOKIE[$config->cookieUserKey()];
      setcookie($config->cookieUserKey(), '', time() - 3600, '/', Cookie::getDomain());
    }

    // Set the Logout cookie for the Wiki, so the user won't see cached pages
    // 5 = $wgClockSkewFudge in the Wiki
    setcookie('wikiLoggedOut', gmdate('YmdHis', time() + 5), (time() + 86400), '/', Cookie::getDomain());

    // delete sessions from DB
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_SESSIONS." WHERE user_id = :user_id");
    $stmt->bindparam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (logout)!');

    if (isset($_REQUEST['target']) && $_REQUEST['target'] != '') {
      header('Location: http://'.$_SERVER['HTTP_HOST'].$_REQUEST['target']);
      exit;
    }

    header('Location: '.$config->pathRosCMS().'?page=my');
    exit;
  } // end of member function login


  /**
   * User Settings:
   * user_setting_multisession == "true" (default: false) [multi sessions are allowed for this user]
   * user_setting_browseragent == "true" (default: true) [no one should deactivate ("false") this option or only if he change the user agent very often (e.g. in opera: IE <=> Opera)]
   * user_setting_ipaddress == "true" (default: true) [IP address check; avoid this setting if the user is behind a proxy or use more than one pc the same time (a possible security risk, but some persons wanted that behavior ...); Note: this is a per user setting, everyone can change it!]
   * user_setting_timeout == "true" (default: false) [NO timeout; so user can use the ros homepage systems without to login everytime]
   *
   * @access public
   */
  public static function required( )
  {
    $thisuser=&ThisUser::getInstance();

    // check if user wants to logout
    if (isset($_POST['logout'])) {
      header('location:?page=logout');
    }

    // get current location (for redirection, if the login succeds)
    $target = $_SERVER[ 'PHP_SELF' ];
    if ( IsSet( $_SERVER[ 'QUERY_STRING' ] ) ) {
      $target .= '?'.$_SERVER[ 'QUERY_STRING' ];
    }

    // get information about script executer
    $user_id = Login::in(Login::REQUIRED, $target);
    if ($user_id == 0) {
      die('Could not Login.');
    }

    // get user data
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, disabled FROM ".ROSCMST_USERS." WHERE id = :user_id LIMIT 1");
    $stmt->bindparam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (login script #1)!');
    $user = $stmt->fetchOnce(PDO::FETCH_ASSOC);
    if($user === false) {
      die('DB error (login script #2)');
    }

    // if the account is NOT enabled; e.g. a reason could be that a member of the admin group has disabled this account because of spamming, etc.
    if ($user['disabled'] == true) { 
      die('Account is not enabled!<br /><br />System message: User account disabled');
    }

    // collect memberships for current user
    $stmt=&DBConnection::getInstance()->prepare(" SELECT a.name_short FROM ".ROSCMST_AREA." a JOIN ".ROSCMST_AREA_ACCESS." r ON r.area_id = a.id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id = r.group_id WHERE m.user_id =:user_id");
    $stmt->bindparam('user_id',$user['id'],PDO::PARAM_INT);
    $stmt->execute();
    while ($area = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $thisuser->addAccess($area['name_short']);
    }

    $thisuser->setData($user);
  } // end of member function require

} // end of Login
?>
