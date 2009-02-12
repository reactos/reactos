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
 * @package html
 * @subpackage user
 */
class HTML_User_Login extends HTML_User
{



  private $err_message=null;
  private $random_string_security=null;



  /**
   *
   *
   * @access public
   */
  public function __construct( )
  {
    session_start();
    $this->checkLogin();

    $config = &RosCMS::getInstance();

    if (isset($_GET['sec']) && $_GET['sec'] == 'security') { 
      $this->random_string_security = $this->makeKey();
      Cookie::write($config->cookieSecure(), $this->random_string_security, 0);
    } 
    else {
      Cookie::write($config->cookieSecure(), '', time() - 3600);
    }

    // register js files
    $this->register_js('md5.js');
    parent::__construct();
  } // end of constructor



  protected function checkLogin( )
  {
    $config = &RosCMS::getInstance();
  
    // show login dialog
    if (empty($_POST[$config->cookieUserName()]) && empty($_POST[$config->cookiePassword()])) {
      $this->loginPage();
    }

    // pw or user are not set
    elseif(empty($_POST[$config->cookieUserName()])) {
      $this->loginPage('Please enter your username!');
    }
    elseif(empty($_POST[$config->cookiePassword()])) {
      $this->loginPage('Please enter your password');
    }

    // try to login the user
    else {
      $user_id = 0;
      $session_id = '';

      if (isset($_COOKIE[$config->cookieSecure()]) && preg_match('/^([a-z0-9]{32})$/', $_COOKIE[$config->cookieSecure()], $matches)) {
        $session_id = $matches[1];
        $session_found = true;
      }
      else {
        $session_id = $this->makeKey();
        $session_found = false;
      }

      // Check username. It should only contain printable ASCII chars
      if (preg_match('/^([ !-~]+)$/', $_POST[$config->cookieUserName()], $matches)) {
        $user_name = $matches[1];
      }
      else {
        $this->loginPage('You have specified an incorrect username.');
        exit;
      }

      // Check password. It should only contain printable ASCII chars
      if (preg_match('/^([ !-~]+)$/', $_POST[$config->cookiePassword()], $matches)) {
        $password = $matches[1];
      }
      else {
        $this->loginPage('You have specified an invalid password.');
        exit;
      }

      // get user data
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, password, logins, disabled, match_session, match_session_expire FROM ".ROSCMST_USERS." WHERE name = :user_name LIMIT 1");
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
        $this->loginPage("You have specified an incorrect or inactive username, or an invalid password.");
        exit;
      }

      // if the account is NOT enabled; e.g. a reason could be that a member of the admin group has disabled this account because of spamming, etc.
      if ($user['disabled'] == true) { 
        $this->loginPage('Account is not activated or disabled!<br /><br />Check your email inbox (and spam folder), maybe you have overseen the activation information.');
        exit;
      }

      // if the user account setting is "multisession" (a by user setting), it is set to "false" by default
      if ($user['match_session'] == false) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_SESSIONS." WHERE user_id = :user_id");
        $stmt->bindParam('user_id',$user['id'],PDO::PARAM_INT);
        $stmt->execute() or die('DB error (user login #3)!');
    
        if ($stmt->fetchColumn() > 0) {
          $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_SESSIONS." WHERE user_id =:user_id");
          $stmt->bindParam('user_id',$roscms_currentuser_id,PDO::PARAM_INT);
          $stmt->execute();
        }
      }

      // At this point, we've passed all checks and we have a valid login check if there's an existing session, if so, end that session
      if (0 != Login::in( Login::OPTIONAL, '')) {
        $stmt=&DBConnection::getInstance()->prepare("DELETE FROM user_sessions WHERE usersession_user_id =:user_id");
        $stmt->bindParam('user_id',$_COOKIE[$config->cookieUserKey()],PDO::PARAM_INT);
        $stmt->execute();
      }

      // save username
      if (isset($_POST['loginoption1']) && $_POST['loginoption1'] == 'save') {
        // save for 5 months
        Cookie::write($config->cookieLoginName(), $user_name, time() + 24*3600*30*5);
      }

      // delete username (cookie)
      else {
        Cookie::write($config->cookieLoginName(), '',time() - 3600);
      }

      // expire = NULL
      if (isset($_POST['loginoption2']) && $_POST['loginoption2'] == 'notimeout' && $user['match_session_expire'] == true) {
        $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SESSIONS." (id, user_id, expires, browseragent, ipaddress) VALUES (:session_id, :user_id, NULL, :useragent, :ip)");
        $cookie_time = 0x7fffffff; // 31.12.1969
      }

      // expire = 'DATE_ADD(NOW(), INTERVAL 60 MINUTE)';
      else {
        $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SESSIONS." (id, user_id, expires, browseragent, ip) VALUES (:session_id, :user_id, DATE_ADD(NOW(), INTERVAL 30 MINUTE), :useragent, :ip)");
        $cookie_time = time() + 30 * 60;
      }

      // Add an entry to the 'user_sessions' table
      $stmt->bindParam('session_id',$session_id,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$user['id'],PDO::PARAM_INT);
      $stmt->bindParam('useragent',$_SERVER['HTTP_USER_AGENT'],PDO::PARAM_STR);
      $stmt->bindParam('ip',$_SERVER['REMOTE_ADDR'],PDO::PARAM_STR);
      $stmt->execute() or die('DB error (user login #4)!');

      // save session_id (cookie)
      Cookie::write($config->cookieUserKey(), $session_id, $cookie_time);

      // Update the login_counter of the specific user
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET modified = NOW(), logins = logins + 1 WHERE id = :user_id");
      $stmt->bindParam('user_id',$user['id'],PDO::PARAM_INT);
      $stmt->execute();

      unset($session_id);

      if (isset($_REQUEST['target'])) {
        header('Location: http://'.$_SERVER['HTTP_HOST'].$_REQUEST['target']);
        exit;
      }

      header('Location: '.$config->pathRosCMS().'?page=my');
      exit;
    }
  } // end of member function body



  /**
   * shows page with login formular
   *
   * @access private
   */
  private function loginPage( $err_message = '' )
  {
    $this->err_message = $err_message;
  }



  /**
   * shows page with login formular
   *
   * @access private
   */
  protected function body( )
  {
    $config = &RosCMS::getInstance();

    $target_clean = '';
    if (isset($_REQUEST['target']) && preg_match('/^(\/[a-zA-Z0-9!$%&,\'()*+\-.\/:;=?@_~]+)$/', $_REQUEST['target'], $matches)) {
      $target_clean = htmlentities($matches[1]);
    }

    echo_strip('
      <form action="'.$config->pathRosCMS().'?page=login" method="post">
        <h1>Login</h1>
        <p>You don\'t have a '.$config->siteName().' account yet? <a href="'.$config->pathRosCMS().'?page=register">Join now</a>, it\'s free and just takes a minute.</p>

        <div class="bubble">
          <div class="corner_TL">
            <div class="corner_TR"></div>
          </div>
          <h2>'.((isset($_GET['sec']) && $_GET['sec'] == 'security') ? 'Secure ' : '').'Login</h2>
          <div class="field">
            <label for="'.$config->cookieUserName().'">Username</label>
            <input type="text" name="'.$config->cookieUserName().'" tabindex="1" id="'.$config->cookieUserName().'" value="');
    if (isset($_POST[$config->cookieUserName()])) {
      echo $_POST[$config->cookieUserName()];
    }
    elseif (isset($_COOKIE[$config->cookieLoginName()])) {
      echo $_COOKIE[$config->cookieLoginName()];
    }
    echo_strip('" maxlength="50" />
      </div>

      <div class="field">
        <label for="'.$config->cookiePassword().'">Password</label>
        <input name="'.$config->cookiePassword().'" type="password" tabindex="2" id="'.$config->cookiePassword().'" maxlength="50" />
      </div>');

    if (empty($_GET['sec']) || $_GET['sec'] == 'standard') {
      echo_strip('
        <fieldset>
          <legend>Login options</legend>
          <input type="checkbox" name="loginoption1" id="loginoption1" value="save"'.(isset($_COOKIE[$config->cookieLoginName()]) ? ' checked="checked"' : '').' tabindex="3" />
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
          <button type="submit" name="submit"'.((isset($_GET['sec']) && $_GET['sec'] == 'security') ? ' onclick="'.$config->cookiePassword().'.value = calcMD5(\''.$this->random_string_security.'\' + calcMD5('.$config->cookiePassword().'.value))"': '').'>Login</button>
        </div>
        <div class="corner_BL">
          <div class="corner_BR"></div>
        </div>
      </div>');

    if ($this->err_message != "") {
      echo_strip('
        <div class="bubble message">
          <div class="corner_TL">
            <div class="corner_TR"></div>
          </div>
          <strong>');echo $this->err_message;echo_strip('</strong>
          <div class="corner_BL">
            <div class="corner_BR"></div>
          </div>
        </div>');
    }

    echo '<div style="margin:10px;text-align:center;">';

    if (empty($_GET['sec']) || $_GET['sec'] == 'standard') {
      echo '<a href="'.$config->pathRosCMS().'?page=login&amp;sec=security'.(($target_clean != '') ? '&amp;target='.urlencode($target_clean) : '').'">Use enhanced security</a>';
    }
    else {
      echo '<a href="'.$config->pathRosCMS().'?page=login&amp;sec=standard'.(($target_clean != '') ? '&amp;target='.urlencode($target_clean) : '').'">Use standard security</a>';
    }
    
    echo_strip('
          <br />
          <a href="'.$config->pathRosCMS().'?page=login&amp;subpage=lost">Lost username or password?</a>
        </div>
      </form>');
  } // end of member function loginForm


  /**
   *
   *
   * @access private
   */
  private function makeKey( )
  {
    $random_string = '';
    for($i = 0; $i < 32; ++$i) {
      // generate a good random string
      mt_srand((double)microtime()*1000000);
      mt_srand((double)microtime()*65000*mt_rand(1,248374));
      $random_string .= chr(mt_rand(97,122)); //32,126
    }

    return $random_string;
  } // end of member function makeKey


} // end of HTML_User_Login
?>
