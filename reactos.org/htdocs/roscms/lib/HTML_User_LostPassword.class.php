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
    global $rdf_uri_3;
    global $roscms_SET_path_ex;
    global $rdf_name, $rdf_name_long;
    global $rdf_register_user_pwd_min, $rdf_register_user_pwd_max;
    global $rdf_support_email_str;

    $err_message = ''; // error message box text
    $mail_exists = false; // email already exists in the database (true = email exists)
    $password_id_exists = null; // pwd-id exists in the database (true = pwd-id exists)

    $activation_code = $rdf_uri_3;
    $mail_exists = isset($_POST['registerpost']) && isset($_POST['useremail']) && $_POST['useremail'] != '' && ROSUser::hasEmail($_POST['useremail']);
    $password_id_exists =  ROSUser::hasPasswordReset($activation_code);

    if (strlen($activation_code > 6)) {
      echo_strip('
        <h1><a href="'.$roscms_SET_path_ex.'login/">Login</a> &gt; Reset your Password</h1>
        <div class="u-h1">Reset your Password</div>
        <div class="u-h2">Have you forgotten your password of your '.$rdf_name.' account? Don\'t panic. You have already requested us that we reset your password. Now it\'s your turn to enter a new password for your '.$rdf_name.' account.</div>');
    }
    else {
      echo_strip('
        <h1><a href="'.$roscms_SET_path_ex.'login/">Login</a> &gt; Lost Username or Password?</h1>
        <div class="u-h1">Lost Username or Password?</div>
        <div class="u-h2">Have you forgotten your username and/or password of your '.$rdf_name.' account? Don\'t panic. We can send you your username and let you reset your password. All you need is your email address.</div>');
    }

    echo_strip('
      <form action="'.$roscms_SET_path_ex.'login/lost/" method="post">
        <div style="text-align: center;">
          <div style="margin:0px auto; background: #e1eafb none repeat scroll 0%; width: 300px;">
            <div class="corner1">
              <div class="corner2">
                <div class="corner3">
                  <div class="corner4">
                    <div style="text-align:center; padding: 4px;">');


    if (strlen($activation_code) > 6) {

      if (!$password_id_exists) {
        echo '<div class="login-title">Invalid Code</div>';
        $err_message = "Nothing for you to see here. <br />Please move along.";
      }
    }

    if (strlen($activation_code) > 6 && isset($_POST['registerpost']) && !empty($_POST['userpwd1']) && !empty($_POST['userpwd2']) && strlen($_POST['userpwd1']) >= $rdf_register_user_pwd_min && strlen($_POST['userpwd1']) < $rdf_register_user_pwd_max && $_POST['userpwd1'] == $_POST['userpwd2'] && !empty($_POST['usercaptcha']) && !empty($_SESSION['rdf_security_code']) && strtolower($_SESSION['rdf_security_code']) == strtolower($_POST['usercaptcha']) && $password_id_exists) {
      $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM users WHERE user_roscms_getpwd_id = :getpwd_id LIMIT 1");
      $stmt->bindParam('getpwd_id',$activation_code,PDO::PARAM_STR);
      $stmt->execute();
      $user_id = $stmt->fetchColumn();

      // set new account password
      $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_roscms_getpwd_id = '', user_roscms_password = MD5( :password ), user_timestamp_touch2 = NOW( ) WHERE user_id = :user_id");
      $stmt->bindParam('password',$_POST['userpwd1'],PDO::PARAM_STR);
      $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
      $stmt->execute();

      echo_strip('
        <div class="login-title">Password changed</div>
        <div><a href="'.$roscms_SET_path_ex.'login/" style="color:red !important; text-decoration:underline;">Login now</a>!</div>');

    }
    elseif (strlen($activation_code) < 6 && isset($_POST['registerpost']) && !empty($_POST['useremail']) && EMail::isValid($_POST['useremail']) && !empty($_POST['usercaptcha']) && !empty($_SESSION['rdf_security_code']) && strtolower($_SESSION['rdf_security_code']) == strtolower($_POST['usercaptcha']) && $mail_exists) {
    
      // password activation code
      $activation_code = ROSUser::makeActivationCode();

      $stmt=DBConnection::getInstance()->prepare("SELECT user_id, user_name FROM users WHERE user_email = :email LIMIT 1");
      $stmt->bindParam('email',$_POST['useremail'],PDO::PARAM_STR);
      $stmt->execute();
      $user = $stmt->fetchOnce();

      // add activation code to account
      $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_roscms_getpwd_id = :getpwd_id, user_timestamp_touch2 = NOW( ) WHERE user_id = :user_id LIMIT 1");
      $stmt->bindParam('getpwd_id',$activation_code,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$user['user_id'],PDO::PARAM_INT);
      $stmt->execute();

      // Email subject
      $subject = $rdf_name_long.' - Lost username or password?';

      // Email message
      $message = $rdf_name_long." - Lost username or password?\n\n\nYou have requested your ".$rdf_name." account login data.\n\nYou haven't requested your account login data? Oops, then someone has tried the 'Lost username or password?' function with your email address, just ignore this email.\n\n\nUsername: ".$user['user_name']."\n\n\nLost your password? Reset your password: ".$roscms_SET_path_ex."login/lost/".$activation_code."/\n\n\nBest regards,\nThe ".$rdf_name." Team\n\n\n(please do not reply as this is an auto generated email!)";

      // send the Email
      if (EMail::send($_POST['useremail'], $subject, $message)) {
        echo_strip('
          <div class="login-title">Account login data sent</div>
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
        echo '<div class="login-title">Reset your Password</div>';
      }
      else {
        echo '<div class="login-title">Lost Username or Password?</div>';
      }
      echo '</div>';

      if (strlen($activation_code) > 6) {
        echo_strip('
          <div class="login-form">
            <label for="userpwd1"'.(isset($_POST['registerpost']) ? ' style="color:red;"' : '').'>New Password</label>
            <input name="userpwd1" type="password" class="input" tabindex="2" id="userpwd1" size="50" maxlength="50" />');

        if ((isset($_POST['userpwd1']) && strlen($_POST['userpwd1']) > $rdf_register_user_name_max)) {
          echo_strip('
            <br />
            <em>Please use a stronger password! At least '.$rdf_register_user_pwd_min.' characters, do not include common words or names, and combine three of these character types: uppercase letters, lowercase letters, numbers, or symbols (ASCII characters).</em>');
        }
        else {
          echo_strip('
            <br />
            <span style="color:#817A71;">uppercase letters, lowercase letters, numbers, and symbols (ASCII characters)</span>');
        }
        
        echo_strip('
          </div>
          <div class="login-form">
            <label for="userpwd2"'.(isset($_POST['registerpost']) ? ' style="color:red;"' : '').'>Re-type Password</label>
            <input name="userpwd2" type="password" class="input" tabindex="3" id="userpwd2" size="50" maxlength="50" />
          </div>');
      }
      else {
        echo_strip('
          <div class="login-form">
            <label for="useremail"'.((isset($_POST['registerpost']) && (empty($_POST['useremail']) || !EMail::isValid($_POST['useremail']))) ? ' style="color:red;"' : '').'>E-Mail</label>
            <input name="useremail" type="text" class="input" tabindex="4" id="useremail"'.(isset($_POST['useremail']) ? 'value="'.$_POST['useremail'].'"' : '').' size="50" maxlength="50" />
          </div>');
      }

      echo_strip('
        <div class="login-form">
          <label for="usercaptcha"'.(isset($_POST['registerpost'])? ' style="color:red;"' :'').'>Type the code shown</label>
          <input name="usercaptcha" type="text" class="input" tabindex="7" id="usercaptcha" size="50" maxlength="50" />');

      echo '
          <script type="text/javascript">
          <!--'."
            var BypassCacheNumber = 0;

            function CaptchaReload()
            {
              ++BypassCacheNumber;
              document.getElementById('captcha').src = '".$roscms_SET_path_ex."register/captcha/' + BypassCacheNumber;
            }

            document.write('".'<br /><span style="color:#817A71;">If you can\'t read this, try <a href="javascript:CaptchaReload()">another one</a>.</span>'."');
          -->".'
          </script>';
      echo_strip('
          <img id="captcha" src="'.$roscms_SET_path_ex.'register/captcha" style="padding-top:10px;" alt="If you can\'t read this, try another one or email '.$rdf_support_email_str.' for help." title="Are you human?" />
          <br />');

      if (isset($_POST['registerpost'])) { 
        echo_strip('
          <br />
          <em>Captcha code is case insensitive. <br />If you can\'t read it, try another one.</em>');
      }

      echo_strip('
        </div>
        <div class="login-button">
          <input type="submit" name="submit" value="Send" tabindex="8" /><br />
          <input type="button" onclick="'."window.location=".$roscms_SET_path_ex."'".'" tabindex="9" value="Cancel" name="cancel" style="color:#777777;" />
          <input name="registerpost" type="hidden" id="registerpost" value="reg" />
        </div>');
    }

    echo_strip('
              </div>
            </div>
          </div>
        </div>
      </div>');

    if ($err_message != '') {
      echo_strip('
        <div style="background: #FAD163 none repeat scroll 0%; width: 300px; margin:10px;">
          <div class="corner1">
            <div class="corner2">
              <div class="corner3">
                <div class="corner4">
                  <div style="text-align:center; padding: 4px;font-weight:bold;">');echo $err_message;echo_strip('</div>
                </div>
              </div>
            </div>
          </div>
        </div>');
    }

    echo_strip('
        </div>
      </form>');
  }


} // end of HTML_User_LostPassword
?>
