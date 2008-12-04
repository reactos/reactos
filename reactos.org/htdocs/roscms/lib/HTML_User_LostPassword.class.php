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
 * class HTML_User_LostPassword
 * 
 */
class HTML_User_LostPassword extends HTML_User
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
    global $roscms_intern_page_link, $roscms_intern_webserver_roscms;
    global $rdf_name, $rdf_name_long;
    global $rdf_register_user_pwd_min, $rdf_register_user_pwd_max;
    global $rdf_support_email_str;

    $err_message = ''; // error message box text
    $mail_exists = false; // email already exists in the database (true = email exists)
    $password_id_exists = null; // pwd-id exists in the database (true = pwd-id exists)

    $activation_code = @$_GET['code'];
    $mail_exists = isset($_POST['registerpost']) && isset($_POST['useremail']) && $_POST['useremail'] != '' && ROSUser::hasEmail($_POST['useremail']);
    $password_id_exists =  ROSUser::hasPasswordReset($activation_code);

    if (strlen($activation_code > 6)) {
      echo_strip('
        <h1><a href="'.$roscms_intern_page_link.'login">Login</a> &gt; Reset your Password</h1>
        <p>Have you forgotten your password of your '.$rdf_name.' account? Don\'t panic. You have already requested us that we reset your password. Now it\'s your turn to enter a new password for your '.$rdf_name.' account.</p>');
    }
    else {
      echo_strip('
        <h1><a href="'.$roscms_intern_page_link.'login">Login</a> &gt; Lost Username or Password?</h1>
        <p>Have you forgotten your username and/or password of your '.$rdf_name.' account? Don\'t panic. We can send you your username and let you reset your password. All you need is your email address.</p>');
    }

    echo_strip('
      <form action="'.$roscms_intern_page_link.'login&amp;subpage=lost" method="post">
        <div class="bubble">
          <div class="corner_TL">
            <div class="corner_TR"></div>
          </div>');


    if (strlen($activation_code) > 6) {

      if (!$password_id_exists) {
        echo '<h2>Invalid Code</h2>';
        $err_message = "Nothing for you to see here. <br />Please move along.";
      }
    }

    if (strlen($activation_code) > 6 && isset($_POST['registerpost']) && !empty($_POST['userpwd1']) && !empty($_POST['userpwd2']) && strlen($_POST['userpwd1']) >= $rdf_register_user_pwd_min && strlen($_POST['userpwd1']) < $rdf_register_user_pwd_max && $_POST['userpwd1'] == $_POST['userpwd2'] && !empty($_POST['usercaptcha']) && !empty($_SESSION['rdf_security_code']) && strtolower($_SESSION['rdf_security_code']) == strtolower($_POST['usercaptcha']) && $password_id_exists) {
      $stmt=DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_USERS." WHERE activation_password = :getpwd_id LIMIT 1");
      $stmt->bindParam('getpwd_id',$activation_code,PDO::PARAM_STR);
      $stmt->execute();
      $user_id = $stmt->fetchColumn();

      // set new account password
      $stmt=DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET activation_password = '', password = MD5( :password ), modified = NOW() WHERE id = :user_id");
      $stmt->bindParam('password',$_POST['userpwd1'],PDO::PARAM_STR);
      $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
      $stmt->execute();

      echo_strip('
        <h2>Password changed</h2>
        <div><a href="'.$roscms_intern_page_link.'login" style="color:red !important; text-decoration:underline;">Login now</a>!</div>');

    }
    elseif (strlen($activation_code) < 6 && isset($_POST['registerpost']) && !empty($_POST['useremail']) && EMail::isValid($_POST['useremail']) && !empty($_POST['usercaptcha']) && !empty($_SESSION['rdf_security_code']) && strtolower($_SESSION['rdf_security_code']) == strtolower($_POST['usercaptcha']) && $mail_exists) {
    
      // password activation code
      $activation_code = ROSUser::makeActivationCode();

      $stmt=DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_USERS." WHERE email = :email LIMIT 1");
      $stmt->bindParam('email',$_POST['useremail'],PDO::PARAM_STR);
      $stmt->execute();
      $user = $stmt->fetchOnce();

      // add activation code to account
      $stmt=DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET activation_password = :getpwd_id WHERE id = :user_id LIMIT 1");
      $stmt->bindParam('getpwd_id',$activation_code,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$user['user_id'],PDO::PARAM_INT);
      $stmt->execute();

      // Email subject
      $subject = $rdf_name_long.' - Lost username or password?';

      // Email message
      $message = $rdf_name_long." - Lost username or password?\n\n\nYou have requested your ".$rdf_name." account login data.\n\nYou haven't requested your account login data? Oops, then someone has tried the 'Lost username or password?' function with your email address, just ignore this email.\n\n\nUsername: ".$user['name']."\n\n\nLost your password? Reset your password: ".$roscms_intern_page_link."login&subpage=lost&code=".$activation_code."/\n\n\nBest regards,\nThe ".$rdf_name." Team\n\n\n(please do not reply as this is an auto generated email!)";

      // send the Email
      if (EMail::send($_POST['useremail'], $subject, $message)) {
        echo_strip('
          <h2>Account login data sent</h2>
          <div>Check your email inbox (and spam folder) for the email that contains your account login data.</div>');
        unset($_SESSION['rdf_security_code']);
        unset($message);
      }
      else {
        $err_message = 'Error while trying to send Email.';
      }
    }
    elseif ($password_id_exists === null || $password_id_exists === true) {

      if (strlen($activation_code) > 6) {
        echo '<h2>Reset your Password</h2>';
      }
      else {
        echo '<h2>Lost Username or Password?</h2>';
      }

      if (strlen($activation_code) > 6) {
        echo_strip('
          <div class="field">
            <label for="userpwd1"'.(isset($_POST['registerpost']) ? ' style="color:red;"' : '').'>New Password</label>
            <input type="password" name="userpwd1" tabindex="2" id="userpwd1" maxlength="50" />');

        if ((isset($_POST['userpwd1']) && strlen($_POST['userpwd1']) > $rdf_register_user_name_max)) {
          echo_strip('
            <br />
            <em>Please use a stronger password! At least '.$rdf_register_user_pwd_min.' characters, do not include common words or names, and combine three of these character types: uppercase letters, lowercase letters, numbers, or symbols (ASCII characters).</em>');
        }
        else {
          echo_strip('
            <br />
            <div class="detail">uppercase letters, lowercase letters, numbers, and symbols (ASCII characters)</div>');
        }
        
        echo_strip('
          </div>
          <div class="field">
            <label for="userpwd2"'.(isset($_POST['registerpost']) ? ' style="color:red;"' : '').'>Re-type Password</label>
            <input type="password" name="userpwd2" tabindex="3" id="userpwd2" maxlength="50" />
          </div>');
      }
      else {
        echo_strip('
          <div class="field">
            <label for="useremail"'.((isset($_POST['registerpost']) && (empty($_POST['useremail']) || !EMail::isValid($_POST['useremail']))) ? ' style="color:red;"' : '').'>E-Mail</label>
            <input name="useremail" type="text" tabindex="4" id="useremail"'.(isset($_POST['useremail']) ? 'value="'.$_POST['useremail'].'"' : '').' maxlength="50" />
          </div>');
      }

      echo_strip('
        <div class="field">
          <label for="usercaptcha"'.(isset($_POST['registerpost'])? ' style="color:red;"' :'').'>Type the code shown</label>
          <input type="text" name="usercaptcha" tabindex="7" id="usercaptcha" maxlength="50" />');

      echo '
          <script type="text/javascript">
          <!--'."
            var BypassCacheNumber = 0;

            function CaptchaReload()
            {
              ++BypassCacheNumber;
              document.getElementById('captcha').src = '".$roscms_intern_page_link."captcha' + BypassCacheNumber;
            }

            document.write('".'<br /><span style="color:#817A71;">If you can\'t read this, try <a href="javascript:CaptchaReload()">another one</a>.</span>'."');
          -->".'
          </script>';
      echo_strip('
          <img id="captcha" src="'.$roscms_intern_page_link.'captcha" style="padding-top:10px;" alt="If you can\'t read this, try another one or email '.$rdf_support_email_str.' for help." title="Are you human?" />
          <br />');

      if (isset($_POST['registerpost'])) { 
        echo_strip('
          <br />
          <em>Captcha code is case insensitive. <br />If you can\'t read it, try another one.</em>');
      }

      echo_strip('
        </div>
        <div class="field">
          <input name="registerpost" type="hidden" id="registerpost" value="reg" />
          <button type="submit" name="submit">Send</button>
          <button type="button" onclick="'."window.location=".$roscms_intern_webserver_roscms."'".'" style="color:#777777;">Cancel</button>
        </div>');
    }

    echo_strip('
          <div class="corner_BL">
            <div class="corner_BR"></div>
          </div>
        </div>
      </form>');

    if ($err_message != '') {
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

  } // end of member function body


} // end of HTML_User_LostPassword
?>
