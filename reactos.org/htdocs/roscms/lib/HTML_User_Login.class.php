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
 * class HTML_User_Login
 * 
 */
class HTML_User_Login extends HTML_User
{

  /**
   *
   *
   * @access public
   */
  public function __construct( )
  {
    session_start();
    parent::__construct();
  }


  /**
   *
   *
   * @access private
   */
  protected function body( )
  {
    global $roscms_SET_path_ex;
    global $rdf_login_cookie_usrkey;
    global $rdf_login_cookie_seckey;
    global $rdf_login_cookie_usrname;
    global $rdf_login_cookie_usrpwd;
    global $rdf_login_cookie_loginname;

    // show login dialog
    if (empty($_POST[$rdf_login_cookie_usrname]) && empty($_POST[$rdf_login_cookie_usrpwd])) {
      self::loginPage();
    }

    // pw or user are not set
    elseif(empty($_POST[$rdf_login_cookie_usrname])) {
      self::loginPage('Please enter your username!');
    }
    elseif(empty($_POST[$rdf_login_cookie_usrpwd])) {
      self::loginPage('Please enter your password');
    }

    // try to login the user
    else {
      $user_id = 0;
      $session_id = '';

      if (isset($_COOKIE[$rdf_login_cookie_seckey]) && preg_match('/^([a-z0-9]{32})$/', $_COOKIE[$rdf_login_cookie_seckey], $matches)) {
        $session_id = $matches[1];
        $session_found = true;
      }
      else {
        $session_id = self::makeKey();
        $session_found = false;
      }

      // Check username. It should only contain printable ASCII chars
      if (preg_match('/^([ !-~]+)$/', $_POST[$rdf_login_cookie_usrname], $matches)) {
        $user_name = $matches[1];
      }
      else {
        self::loginPage('You have specified an incorrect username.');
        exit;
      }

      // Check password. It should only contain printable ASCII chars
      if (preg_match('/^([ !-~]+)$/', $_POST[$rdf_login_cookie_usrpwd], $matches)) {
        $password = $matches[1];
      }
      else {
        self::loginPage('You have specified an invalid password.');
        exit;
      }

      // get user data
      $stmt=DBConnection::getInstance()->prepare("SELECT user_id, user_roscms_password, user_login_counter, user_account_enabled, user_setting_multisession, user_setting_timeout FROM users WHERE user_name = :user_name LIMIT 1");
      $stmt->bindParam('user_name',$user_name,PDO::PARAM_STR);
      $stmt->execute() or die('DB error (user login #1)!');
      $user = $stmt->fetchOnce(); 

      if ($session_found === true) {
        // we simply concatenate the password and random key to create a unique session md5 hash
        // hmac functions are not available on most web servers, but this is near as dammit.
        $user['user_roscms_password'] = md5($session_id.$user['user_roscms_password']);
        $a_password = $password;
      }
      else {
        $a_password = md5($password);
      }

      if ($a_password != $user['user_roscms_password']) {
        self::loginPage("You have specified an incorrect or inactive username, or an invalid password.");
        exit;
      }

      // if the account is NOT enabled; e.g. a reason could be that a member of the admin group has disabled this account because of spamming, etc.
      if ($user['user_account_enabled'] != 'yes') { 
        self::loginPage('Account is not activated or disabled!<br /><br />Check your email inbox (and spam folder), maybe you have overseen the activation information.');
        exit;
      }

      // if the user account setting is "multisession" (a by user setting), it is set to "false" by default
      if ($user['user_setting_multisession'] == false) {
        $stmt=DBConnection::getInstance()->prepare("SELECT COUNT('usersession_user_id') FROM user_sessions WHERE usersession_user_id = :user_id");
        $stmt->bindParam('user_id',$user['user_id'],PDO::PARAM_INT);
        $stmt->execute() or die('DB error (user login #3)!');
    
        if ($stmt->fetchColumn() > 0) {
          $stmt=DBConnection::getInstance()->prepare("DELETE FROM user_sessions WHERE usersession_user_id =:user_id");
          $stmt->bindParam('user_id',$roscms_currentuser_id,PDO::PARAM_INT);
          $stmt->execute();
        }
      }

      // At this point, we've passed all checks and we have a valid login check if there's an existing session, if so, end that session
      if (0 != Login::in( Login::OPTIONAL, '')) {
        $stmt=DBConnection::getInstance()->prepare("DELETE FROM user_sessions WHERE usersession_user_id =:user_id");
        $stmt->bindParam('user_id',$_COOKIE[$rdf_login_cookie_usrkey],PDO::PARAM_INT);
        $stmt->execute();
      }

      // save username
      if (isset($_POST['loginoption1']) && $_POST['loginoption1'] == 'save') {
        // save for 5 months
        setcookie($rdf_login_cookie_loginname, $user_name, time() + 24*3600*30*5, '/', Cookie::getDomain());
      }

      // delete username (cookie)
      else {
        setcookie($rdf_login_cookie_loginname, '',time() - 3600, '/', Cookie::getDomain());
      }

      // expire = NULL
      if (isset($_POST['loginoption2']) && $_POST['loginoption2'] == 'notimeout' && $user['user_setting_timeout'] == 'true') {
        $stmt=DBConnection::getInstance()->prepare("INSERT INTO user_sessions (usersession_id, usersession_user_id, usersession_expires, usersession_browseragent, usersession_ipaddress) VALUES (:session_id, :user_id, NULL, :useragent, :ip)");
        $cookie_time = 0x7fffffff; // 31.12.1969
      }

      // expire = 'DATE_ADD(NOW(), INTERVAL 60 MINUTE)';
      else {
        $stmt=DBConnection::getInstance()->prepare("INSERT INTO user_sessions (usersession_id, usersession_user_id, usersession_expires, usersession_browseragent, usersession_ipaddress) VALUES (:session_id, :user_id, DATE_ADD(NOW(), INTERVAL 60 MINUTE), :useragent, :ip)");
        $cookie_time = time() + 60 * 60;
      }

      // Add an entry to the 'user_sessions' table
      $stmt->bindParam('session_id',$session_id,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$user['user_id'],PDO::PARAM_INT);
      $stmt->bindParam('useragent',$_SERVER['HTTP_USER_AGENT'],PDO::PARAM_STR);
      $stmt->bindParam('ip',$_SERVER['REMOTE_ADDR'],PDO::PARAM_STR);
      $stmt->execute() or die('DB error (user login #4)!');

      // save session_id (cookie)
      setcookie($rdf_login_cookie_usrkey, $session_id, $cookie_time, '/', Cookie::getDomain());

      // Update the login_counter of the specific user
      $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_timestamp_touch = NOW(), user_login_counter = user_login_counter + 1 WHERE user_id = :user_id");
      $stmt->bindParam('user_id',$user['user_id'],PDO::PARAM_INT);
      $stmt->execute();

      unset($session_id);

      if (isset($_REQUEST['target'])) {
        header('Location: http://'.$_SERVER['HTTP_HOST'].$_REQUEST['target']);
        exit;
      }

      header('Location: '.$roscms_SET_path_ex.'my/');
      exit;
    }
  } // end of member function body


  /**
   *
   *
   * @access private
   */
  private function loginPage( $err_message = '' )
  {
    global $rdf_login_cookie_seckey;
    global $rdf_login_cookie_usrname;
    global $rdf_login_cookie_usrpwd;
    global $rdf_login_cookie_loginname;
    global $rdf_name;
    global $roscms_SET_path_ex;

    //@ADD comment -> why do we need this
    $random_string_security = ''; 

    if (isset($_GET['sec']) && $_GET['sec'] == "security") { 
      $random_string_security = self::makeKey();
      setcookie($rdf_login_cookie_seckey, $random_string_security, 0, '/', Cookie::getDomain());
      echo '<script language="javascript" src="js/md5.js"></script>';
    } 
    else {
      setcookie($rdf_login_cookie_seckey, '', time() - 3600, '/', Cookie::getDomain());
    }

    $target_clean = '';
    if (isset($_REQUEST['target']) && preg_match('/^(\/[a-zA-Z0-9!$%&,\'()*+\-.\/:;=?@_~]+)$/', $_REQUEST['target'], $matches)) {
      $target_clean = $matches[1];
    }

    echo '<form action="'.$roscms_SET_path_ex.'login/" method="post">';

    if ($target_clean != '' ) {
      echo '<input type="hidden" name="target" value="'.$target_clean.'" />';
    }

    echo_strip('
      <h1>Login</h1>
      <div class="u-h1">Login to '.$rdf_name.'</div>
      <div class="u-h2">You don\'t have a '.$rdf_name.' account yet? <a href="'.$roscms_SET_path_ex.'register/">Join now</a>, it\'s free and just takes a minute.</div>
      <div align="center">
      <div style="background: #e1eafb none repeat scroll 0%; width: 300px;">
        <div class="corner1">
          <div class="corner2">
            <div class="corner3">
              <div class="corner4">
                <div style="text-align:center; padding: 4px;">
                  <div class="login-title">'.((isset($_GET['sec']) && $_GET['sec'] == 'security') ? 'Secure ' : '').'Login</div>
                  <div class="login-form">
                    <label for="'.$rdf_login_cookie_usrname.'">Username</label>
                    <input name="'.$rdf_login_cookie_usrname.'" type="text" class="input" tabindex="1" id="'.$rdf_login_cookie_usrname.'" ');
    if (isset($_POST[$rdf_login_cookie_usrname])) {
      echo 'value="'.$_POST[$rdf_login_cookie_usrname].'"';
    }
    elseif (isset($_COOKIE[$rdf_login_cookie_loginname])) {
      echo 'value="' . $_COOKIE[$rdf_login_cookie_loginname].'"';
    }
    echo_strip(' size="50" maxlength="50" />
      </div>

      <div class="login-form">
        <label for="'.$rdf_login_cookie_usrpwd.'">Password</label>
        <input name="'.$rdf_login_cookie_usrpwd.'" type="password" class="input" tabindex="2" id="'.$rdf_login_cookie_usrpwd.'" size="50" maxlength="50" />
      </div>');

    if (isset($_GET['sec']) && ($_GET['sec'] == '' || $_GET['sec'] == 'standard')) {
      echo_strip('
        <div class="login-options">
          <input name="loginoption1" type="checkbox" id="loginoption1" value="save"'.(isset($_COOKIE[$rdf_login_cookie_loginname]) ? 'checked' : '').' tabindex="3" />
          <label for="loginoption1">Save username</label>
          <br />
          <input name="loginoption2" type="checkbox" id="loginoption2" value="notimeout" tabindex="4" /> 
          <label for="loginoption2">Log me on automatically</label>
        </div>');
    }

    echo_strip( '
                  <div class="login-button">
                    <input type="submit" name="submit" value="Login" class="button" tabindex="5"'.((isset($_GET['sec']) && $_GET['sec'] == 'security') ? 'onclick="'.$rdf_login_cookie_usrpwd.'.value = calcMD5(\''.$random_string_security.'\' + calcMD5('.$rdf_login_cookie_usrpwd.'.value))"': '').' />
                    <input name="logintype" type="hidden" id="logintype" value="'.((isset($_GET['sec']) && $_GET['sec'] == 'security') ? 'security' : 'standard').'" />
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>');

    if ($err_message != "") {
      echo_strip('
        <div style="background: #FAD163 none repeat scroll 0%; width: 300px; margin:10px;">
          <div class="corner1">
            <div class="corner2">
              <div class="corner3">
                <div class="corner4">
                  <div style="text-align:center; padding: 4px; font-weight: bold;">');echo $err_message;echo_strip('</div>
                </div>
              </div>
            </div>
          </div>
        </div>');
    }

    echo '<div style="margin:10px">';

    if (isset($_GET['sec']) && ($_GET['sec'] == '' || $_GET['sec'] == 'standard')) {
      echo '<a href="'.$roscms_SET_path_ex.'login/?sec=security'.(($target_clean != '') ? '&amp;target='.urlencode($target_clean) : '').'">Use enhanced security</a>';
    }
    else {
      echo '<a href="'.$roscms_SET_path_ex.'login/?sec=standard'.(($target_clean != '') ? '&amp;target='.urlencode($target_clean) : '').'">Use standard security</a>';
    }
    
    echo_strip('
            </div>
          <a href="'.$roscms_SET_path_ex.'login/lost/">Lost username or password?</a>
        </div>
      </form>');
  } // end of member function


  /**
   *
   *
   * @access private
   */
  private function makeKey( )
  {
    $random_string = '';
    for($i = 0; $i < 32; $i++) {
      // generate a good random string
      mt_srand((double)microtime()*1000000);
      mt_srand((double)microtime()*65000*mt_rand(1,248374));
      $random_string .= chr(mt_rand(97,122)); //32,126
    }

    return $random_string;
  }


} // end of HTML_User_Login
?>
