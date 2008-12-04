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

    // register js files
    $this->register_js('md5.js');
    parent::__construct();
  }


  /**
   *
   *
   * @access private
   */
  protected function body( )
  {
    global $roscms_intern_page_link;
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
      $stmt=DBConnection::getInstance()->prepare("SELECT id, password, logins, disabled, match_session, match_session_expire FROM ".ROSCMST_USERS." WHERE name = :user_name LIMIT 1");
      $stmt->bindParam('user_name',$user_name,PDO::PARAM_STR);
      $stmt->execute() or die('DB error (user login #1)!');
      $user = $stmt->fetchOnce(); 

      if ($session_found === true) {
        // we simply concatenate the password and random key to create a unique session md5 hash
        // hmac functions are not available on most web servers, but this is near as dammit.
        $user['password'] = md5($session_id.$user['password']);
        $a_password = $password;
      }
      else {
        $a_password = md5($password);
      }

      if ($a_password != $user['password']) {
        self::loginPage("You have specified an incorrect or inactive username, or an invalid password.");
        exit;
      }

      // if the account is NOT enabled; e.g. a reason could be that a member of the admin group has disabled this account because of spamming, etc.
      if ($user['disabled'] == true) { 
        self::loginPage('Account is not activated or disabled!<br /><br />Check your email inbox (and spam folder), maybe you have overseen the activation information.');
        exit;
      }

      // if the user account setting is "multisession" (a by user setting), it is set to "false" by default
      if ($user['match_session'] == false) {
        $stmt=DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_SESSIONS." WHERE user_id = :user_id");
        $stmt->bindParam('user_id',$user['id'],PDO::PARAM_INT);
        $stmt->execute() or die('DB error (user login #3)!');
    
        if ($stmt->fetchColumn() > 0) {
          $stmt=DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_SESSIONS." WHERE user_id =:user_id");
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
      if (isset($_POST['loginoption2']) && $_POST['loginoption2'] == 'notimeout' && $user['match_session_expire'] == true) {
        $stmt=DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SESSIONS." (id, user_id, expires, browseragent, ipaddress) VALUES (:session_id, :user_id, NULL, :useragent, :ip)");
        $cookie_time = 0x7fffffff; // 31.12.1969
      }

      // expire = 'DATE_ADD(NOW(), INTERVAL 60 MINUTE)';
      else {
        $stmt=DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SESSIONS." (id, user_id, expires, browseragent, ip) VALUES (:session_id, :user_id, DATE_ADD(NOW(), INTERVAL 30 MINUTE), :useragent, :ip)");
        $cookie_time = time() + 30 * 60;
      }

      // Add an entry to the 'user_sessions' table
      $stmt->bindParam('session_id',$session_id,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$user['id'],PDO::PARAM_INT);
      $stmt->bindParam('useragent',$_SERVER['HTTP_USER_AGENT'],PDO::PARAM_STR);
      $stmt->bindParam('ip',$_SERVER['REMOTE_ADDR'],PDO::PARAM_STR);
      $stmt->execute() or die('DB error (user login #4)!');

      // save session_id (cookie)
      setcookie($rdf_login_cookie_usrkey, $session_id, $cookie_time, '/', Cookie::getDomain());

      // Update the login_counter of the specific user
      $stmt=DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET modified = NOW(), logins = logins + 1 WHERE id = :user_id");
      $stmt->bindParam('user_id',$user['id'],PDO::PARAM_INT);
      $stmt->execute();

      unset($session_id);

      if (isset($_REQUEST['target'])) {
        header('Location: http://'.$_SERVER['HTTP_HOST'].$_REQUEST['target']);
        exit;
      }

      header('Location: '.$roscms_intern_page_link.'my');
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
    global $roscms_intern_page_link;

    //@ADD comment -> why do we need this
    $random_string_security = ''; 

    if (isset($_GET['sec']) && $_GET['sec'] == 'security') { 
      $random_string_security = self::makeKey();
      setcookie($rdf_login_cookie_seckey, $random_string_security, 0, '/', Cookie::getDomain());
    } 
    else {
      setcookie($rdf_login_cookie_seckey, '', time() - 3600, '/', Cookie::getDomain());
    }

    $target_clean = '';
    if (isset($_REQUEST['target']) && preg_match('/^(\/[a-zA-Z0-9!$%&,\'()*+\-.\/:;=?@_~]+)$/', $_REQUEST['target'], $matches)) {
      $target_clean = htmlentities($matches[1]);
    }

    echo_strip('
      <form action="'.$roscms_intern_page_link.'login" method="post">
        <h1>Login</h1>
        <p>You don\'t have a '.$rdf_name.' account yet? <a href="'.$roscms_intern_page_link.'register">Join now</a>, it\'s free and just takes a minute.</p>

        <div class="bubble">
          <div class="corner_TL">
            <div class="corner_TR"></div>
          </div>
          <h2>'.((isset($_GET['sec']) && $_GET['sec'] == 'security') ? 'Secure ' : '').'Login</h2>
          <div class="field">
            <label for="'.$rdf_login_cookie_usrname.'">Username</label>
            <input type="text" name="'.$rdf_login_cookie_usrname.'" tabindex="1" id="'.$rdf_login_cookie_usrname.'" value="');
    if (isset($_POST[$rdf_login_cookie_usrname])) {
      echo $_POST[$rdf_login_cookie_usrname];
    }
    elseif (isset($_COOKIE[$rdf_login_cookie_loginname])) {
      echo $_COOKIE[$rdf_login_cookie_loginname];
    }
    echo_strip('" maxlength="50" />
      </div>

      <div class="field">
        <label for="'.$rdf_login_cookie_usrpwd.'">Password</label>
        <input name="'.$rdf_login_cookie_usrpwd.'" type="password" tabindex="2" id="'.$rdf_login_cookie_usrpwd.'" maxlength="50" />
      </div>');

    if (empty($_GET['sec']) || $_GET['sec'] == 'standard') {
      echo_strip('
        <fieldset>
          <legend>Login options</legend>
          <input type="checkbox" name="loginoption1" id="loginoption1" value="save"'.(isset($_COOKIE[$rdf_login_cookie_loginname]) ? ' checked="checked"' : '').' tabindex="3" />
          <label for="loginoption1">Save username</label>
          <br />
          <input name="loginoption2" type="checkbox" id="loginoption2" value="notimeout" tabindex="4" /> 
          <label for="loginoption2">Log me on automatically</label>
        </fieldset>');
    }

    echo '<div class="field">';

    if ($target_clean != '' ) {
      echo '<input type="hidden" name="target" value="'.$target_clean.'" />';
    }
    echo_strip('
          <input name="logintype" type="hidden" id="logintype" value="'.((isset($_GET['sec']) && $_GET['sec'] == 'security') ? 'security' : 'standard').'" />
          <button type="submit" name="submit"'.((isset($_GET['sec']) && $_GET['sec'] == 'security') ? ' onclick="'.$rdf_login_cookie_usrpwd.'.value = calcMD5(\''.$random_string_security.'\' + calcMD5('.$rdf_login_cookie_usrpwd.'.value))"': '').'>Login</button>
        </div>
        <div class="corner_BL">
          <div class="corner_BR"></div>
        </div>
      </div>');

    if ($err_message != "") {
      echo_strip('
        <div class="bubble message">
          <div class="corner_TL">
            <div class="corner_TR"></div>
          </div>
          <strong>');echo $err_message;echo_strip('</strong>
          <div class="corner_BL">
            <div class="corner_BR"></div>
          </div>
        </div>');
    }

    echo '<div style="margin:10px;text-align:center;">';

    if (empty($_GET['sec']) || $_GET['sec'] == 'standard') {
      echo '<a href="'.$roscms_intern_page_link.'login&amp;sec=security'.(($target_clean != '') ? '&amp;target='.urlencode($target_clean) : '').'">Use enhanced security</a>';
    }
    else {
      echo '<a href="'.$roscms_intern_page_link.'login&amp;sec=standard'.(($target_clean != '') ? '&amp;target='.urlencode($target_clean) : '').'">Use standard security</a>';
    }
    
    echo_strip('
          <br />
          <a href="'.$roscms_intern_page_link.'login&amp;subpage=lost">Lost username or password?</a>
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
