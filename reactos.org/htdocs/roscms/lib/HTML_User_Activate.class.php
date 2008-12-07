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
    $err_message = ''; // error message box text
    $mail_exists = false; // email already exists in the database (true = email exists)
    $activation_code_exists = false; // pwd-id exists in the database (true = pwd-id exists)

    $activation_code = @$_GET['code'];

    echo_strip('
      <h1>Activate '.RosCMS::getInstance()->siteName().' Account</h1>
      <p>Already a member? <a href="'.RosCMS::getInstance()->pathRosCMS().'?page=login">Login now</a>!</p>
      <p>Don\'t have a '.RosCMS::getInstance()->siteName().' account yet? <a href="'.RosCMS::getInstance()->pathRosCMS().'?page=register">Join now</a>, it\'s free and just takes a minute.</p>
      <form action="'.RosCMS::getInstance()->pathRosCMS().'?page=login&amp;subpage=activate" method="post">
        <div class="bubble">
          <div class="corner_TL">
            <div class="corner_TR"></div>
          </div>');

    if (isset($_POST['registerpost']) && isset($_POST['useremail']) && $_POST['useremail'] != '') {

      // check if another account with the same email address already exists
      $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_USERS." WHERE email = :email LIMIT 1");
      $stmt->bindParam('email',$_POST['useremail'],PDO::PARAM_STR);
      $stmt->execute();
      $mail_exists = ($stmt->fetchColum() !== false);
    }

    if (strlen($activation_code) > 6) {

      // check if an account with the pwd-id exists
      $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_USERS." WHERE activation = :activation_code LIMIT 1");
      $stmt->bindParam('activation_code',$activation_code,PDO::PARAM_STR);
      $stmt->execute();

      if ($stmt->fetchColumn() !== false) {
        $activation_code_exists = true;
      }
      else {
        echo '<h2>Invalid Code</h2>';
        $err_message = 'Nothing for you to see here. <br />Please move along.';
      }
    }
    else {
      echo '<h2>Invalid Code</h2>';
      $err_message = 'Nothing for you to see here. <br />Please move along.';
    }

    if (strlen($activation_code) > 6 && isset($_POST['registerpost']) && isset($_POST['useremail']) && EMail::isValid($_POST['useremail']) && $activation_code_exists && $mail_exists) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_USERS." WHERE activation = :activation_code LIMIT 1");
      $stmt->bindParam('activation_code',$activation_code,PDO::PARAM_STR);
      $stmt->execute();
      $user_id = $stmt->fetchColumn();

      // set new account password
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET activation = '', modified = NOW() WHERE id = :user_id LIMIT 1");
      $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
      $stmt->execute();

      echo_strip('
        <h2>Account activated</h2>
        <div><a href="'.RosCMS::getInstance()->pathRosCMS().'?page=login" style="color:red !important; text-decoration:underline;">Login now</a>!</div>');
    }
    elseif ($activation_code_exists) {
      echo_strip('
          <h2>Activate your Account</h2>
        </div>

        <div class="field">
          <label for="useremail"'.((isset($_POST['registerpost']) && (EMail::isValid($_POST['useremail']) || !$mail_exists)) ? ' style="color:red;"' : '').'>Your E-Mail Address</label>
          <input type="text" name="useremail" tabindex="4" id="useremail"'.(isset($_POST['useremail']) ? 'value="'.$_POST['useremail'].'"' : '').' maxlength="50" />
        </div>

        <div class="field">
          <input name="registerpost" type="hidden" id="registerpost" value="reg" />
          <button type="submit" name="submit">Activate Account</button>
        </div>');
    }

    echo_strip('
        <div class="corner_BL">
          <div class="corner_BR"></div>
        </div>
      </div>');

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

    echo_strip('
      </form>');
  }


} // end of HTML_User_Activate
?>
