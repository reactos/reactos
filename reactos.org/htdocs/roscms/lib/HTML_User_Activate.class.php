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
 * class HTML_User_Activate
 * 
 */
class HTML_User_Activate extends HTML_User
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
    global $rdf_logon_system_name;

    $err_message = ''; // error message box text
    $mail_exists = false; // email already exists in the database (true = email exists)
    $activation_code_exists = false; // pwd-id exists in the database (true = pwd-id exists)

    $activation_code = $rdf_uri_3;

    echo_strip('
      <h1>Activate myReactOS Account</h1>
      <div class="u-h1">Activate myReactOS Account</div>
      <div class="u-h2">Already a member? <a href="'.$roscms_SET_path_ex.'login/">Login now</a>!<br />
        Don\'t have a '.$rdf_logon_system_name.' account yet? <a href="'.$roscms_SET_path_ex.'register/">Join now</a>, it\'s free and just takes a minute.</div>
      <form action="'.$roscms_SET_path_ex.'login/activate/" method="post">
        <div align="center">
          <div style="background: #e1eafb none repeat scroll 0%; width: 300px;">
            <div class="corner1">
              <div class="corner2">
                <div class="corner3">
                  <div class="corner4">
                    <div style="text-align:center; padding: 4px;">');

    if (isset($_POST['registerpost']) && isset($_POST['useremail']) && $_POST['useremail'] != '') {

      // check if another account with the same email address already exists
      $stmt=DBConnection::getInstance()->prepare("SELECT user_email FROM users WHERE user_email = :email LIMIT 1");
      $stmt->bindParam('email',$_POST['useremail'],PDO::PARAM_STR);
      $stmt->execute();
      $mail_exists = ($stmt->fetchColum() !== false);
    }

    if (strlen($activation_code) > 6) {

      // check if an account with the pwd-id exists
      $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM users WHERE user_register_activation = :activation_code LIMIT 1");
      $stmt->bindParam('activation_code',$activation_code,PDO::PARAM_STR);
      $stmt->execute();

      if ($stmt->fetchColumn() !== false) {
        $activation_code_exists = true;
      }
      else {
        echo '<div class="login-title">Invalid Code</div>';
        $err_message = 'Nothing for you to see here. <br />Please move along.';
      }
    }
    else {
      echo '<div class="login-title">Invalid Code</div>';
      $err_message = 'Nothing for you to see here. <br />Please move along.';
    }

    if (strlen($activation_code) > 6 && isset($_POST['registerpost']) && isset($_POST['useremail']) && EMail::isValid($_POST['useremail']) && $activation_code_exists && $mail_exists) {
      $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM users WHERE user_register_activation = :activation_code LIMIT 1");
      $stmt->bindParam('activation_code',$activation_code,PDO::PARAM_STR);
      $stmt->execute();
      $user_id = $stmt->fetchColumn();

      // set new account password
      $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_register_activation = '', user_account_enabled = 'yes', user_timestamp_touch2 = NOW() WHERE user_id = :user_id LIMIT 1");
      $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
      $stmt->execute();

      echo_strip('
        <div class="login-title">Account activated</div>
        <div><a href="'.$roscms_SET_path_ex.'login/" style="color:red !important; text-decoration:underline;">Login now</a>!</div>');
    }
    elseif ($activation_code_exists) {
      echo_strip('
          <div class="login-title">Activate your Account</div>
        </div>

        <div class="login-form">
          <label for="useremail"'.((isset($_POST['registerpost']) && (EMail::isValid($_POST['useremail']) || !$mail_exists)) ? ' style="color:red;"' : '').'>Your E-Mail Address</label>
          <input name="useremail" type="text" class="input" tabindex="4" id="useremail"'.(isset($_POST['useremail']) ? 'value="'.$_POST['useremail'].'"' : '').' size="50" maxlength="50" />
        </div>

        <div class="login-button">
          <input type="submit" name="submit" value="Activate Account" tabindex="8" /><br />
          <input name="registerpost" type="hidden" id="registerpost" value="reg" />
        </div>');
    }

    echo_strip('
                </div>
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
                  <div style="text-align:center; padding: 4px;">');echo $err_message;echo_strip('</div>
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


} // end of HTML_User_Activate
?>
