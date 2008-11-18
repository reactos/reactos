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
 * class HTML_User_Profile_Edit
 * 
 */
class HTML_User_ProfileEdit extends HTML_User
{

  /**
   *
   *
   * @access public
   */
  public function __construct( $page_title = '')
  {
    require('login.php');
    session_start();
    parent::__construct( $page_title);
  }


  /**
   *
   *
   * @access private
   */
  protected function body( )
  {
    global $roscms_intern_account_id;
    global $roscms_SET_path_ex;
    global $rdf_uri_3;
    global $rdf_name_long;
    global $rdf_name;
    global $rdf_register_user_pwd_min;
    global $rdf_register_user_pwd_max;
    global $roscms_intern_webserver_pages;

    $activation_code = $rdf_uri_3;

    $err_message = ''; // error message box text
    $existemail = false; // email already exists in the database (true = email exists)
    $safepwd = ''; // unsafe password, common cracked passwords ("" = not checked; "true" = fine; "false" =  match with a db entry => protected name)
    $password_change = false; // new password

    if ($activation_code != '' && strlen($activation_code) > 6) {
      echo_strip('
        <h1><a href="<?php echo $roscms_SET_path_ex; ?>my/">myReactOS</a> &gt; <a href="<?php echo $roscms_SET_path_ex; ?>my/edit/">Edit My Profile</a> &gt; Activate E-Mail Address</h1>
        <div class="u-h1">Activate E-Mail Address</div>
        <div class="u-h2">
          So you have a new email address and would like to keep your <?php echo $rdf_name; ?> account up-to-date? That is a very good idea. To confirm your email address change, please enter your new email address again.
        </div>');
    }
    else {
      echo_strip('
        <h1><a href="'.$roscms_SET_path_ex.'my/">myReactOS</a> &gt; Edit My Profile</h1>
        <div class="u-h1">Edit My Profile</div>
        <div class="u-h2">Update your user account profile data to reflect the current state.</div>');
    }
    
    echo_strip('
      <form action="'.$roscms_SET_path_ex.'my/edit/" method="post">
        <div align="center">
          <div style="background: #e1eafb none repeat scroll 0%; width: 300px;">
            <div class="corner1">
              <div class="corner2">
                <div class="corner3">
                  <div class="corner4">
                    <div style="text-align:center; padding: 4px; ">');

    $stmt=DBConnection::getInstance()->prepare("SELECT user_id, user_name, user_fullname, user_email, user_email_activation, user_website, user_country, user_timezone, user_occupation, user_setting_multisession, user_setting_browseragent, user_setting_ipaddress, user_setting_timeout FROM users WHERE user_id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
    $stmt->execute();
    $profile = $stmt->fetchOnce();

    // DB update E-Mail adress
    if ($this->checkEmailUpdate($activation_code, $profile['user_email_activation'])) {
      $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_timestamp_touch2 = NOW() , user_email_activation = '', user_email = :email WHERE user_id = :user_id LIMIT 1");
      $stmt->bindParam('user_id',$profile['user_id'],PDO::PARAM_INT);
      $stmt->bindParam('email',$_POST['useremail'],PDO::PARAM_STR);
      $stmt->execute();

      ROSUser::syncSubsystems($profile['user_id']);

      echo_strip('
        <div class="login-title">E-Mail Address Changed</div>
        <div>
          <a href="'.$roscms_SET_path_ex.'my/" style="color: red !important; text-decoration:underline;">My Profile</a>
        </div>');
      return;
    }

    // wanna change password
    if ($this->checkPasswordUpdate()) {
      $stmt=DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM user_unsafepwds WHERE pwd_name = :pwd_name");
      $stmt->bindParam('pwd_name',$_POST['userpwd1']);
      $stmt->execute();
      if ($stmt->fetchColumn() == 0) {
        $safepwd = true; 
      }
      else {
        $safepwd = false; 
      }
    }

    // wanna change email
    if (isset($_POST['registerpost']) && isset($_POST['useremail']) && $_POST['useremail'] != '') {
    
      // check if another account with the same email address already exists
      $stmt=DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM users WHERE user_email = :email");
      $stmt->bindParam('email',$_POST['useremail'],PDO::PARAM_STR);
      $stmt->execute();

      if ($stmt->fetchColumn() && $profile['user_email'] != $_POST['useremail']) {
       $existemail = true;
      }
    }

    if (($activation_code == '' || strlen($activation_code) <= 6) && isset($_POST['registerpost']) && ($safepwd === true || $safepwd === '') && (isset($_POST['userpwd1']) && ($_POST['userpwd1'] == "" || (strlen($_POST['userpwd1']) >= $rdf_register_user_pwd_min && strlen($_POST['userpwd1']) < $rdf_register_user_pwd_max))) && isset($_POST['useremail']) && EMail::isValid($_POST['useremail']) && !$existemail) {

      // user language (browser settings)
      $userlang = Language::checkStatic($_SERVER["HTTP_ACCEPT_LANGUAGE"]);

      // email address activation code
      $s = '';
      for ($n=0; $n<20; $n++) {
        $s .= chr(rand(0, 255));
      }
      $s = base64_encode($s);   // base64-set, but filter out unwanted chars
      $s = preg_replace('/[+\/=IG0ODQRtl]/i', '', $s);  // strips hard to discern letters, depends on used font type

      $s = substr($s, 0, rand(10, 15));
      $account_act_code = $s;

      // update password
      if ($safepwd === true) {
        $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_roscms_password = MD5(:password) WHERE user_id = :user_id LIMIT 1");
        $stmt->bindParam('password',$_POST['userpwd1'],PDO::PARAM_STR);
        $stmt->bindParam('user_id',$profile['user_id'],PDO::PARAM_INT);
        $password_change = $stmt->execute();

        $password_change = true;
      }

      // set email activation code
      if ($profile['user_email'] != $_POST['useremail']) {
        $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_email_activation = :activation_code WHERE user_id = :user_id LIMIT 1");
        $stmt->bindValue('activation_code',htmlspecialchars($_POST['useremail']).$account_act_code,PDO::PARAM_STR);
        $stmt->bindParam('user_id',$profile['user_id'],PDO::PARAM_INT);
        $stmt->execute();
      }

      // update account data
      $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_timestamp_touch2 = NOW( ) , user_fullname = :fullname, user_website = :website, user_language = :language, user_country = :country, user_timezone = :timezone, user_occupation = :occupation, user_setting_multisession = :setting_multisession, user_setting_browseragent = :setting_browser, user_setting_ipaddress = :setting_ip, user_setting_timeout = :setting_timeout WHERE user_id = :user_id LIMIT 1");
      $stmt->bindParam('fullname',htmlspecialchars($_POST['userfullname']),PDO::PARAM_STR);
      $stmt->bindParam('website',$_POST['userwebsite'],PDO::PARAM_STR);
      $stmt->bindParam('language',$userlang,PDO::PARAM_STR);
      $stmt->bindParam('country',$_POST['country'],PDO::PARAM_STR);
      $stmt->bindParam('timezone',$_POST['tzone'],PDO::PARAM_STR);
      $stmt->bindParam('occupation',$_POST['useroccupation'],PDO::PARAM_STR);
      $stmt->bindValue('setting_multisession',isset($_POST['loginoption1']),PDO::PARAM_STR);
      $stmt->bindValue('setting_browser',isset($_POST['loginoption2']),PDO::PARAM_STR);
      $stmt->bindValue('setting_ip',isset($_POST['loginoption3']),PDO::PARAM_STR);
      $stmt->bindValue('setting_timeout',isset($_POST['loginoption4']),PDO::PARAM_STR);
      $stmt->bindParam('user_id',$profile['user_id'],PDO::PARAM_INT);
      $stmt->execute();

      echo '<div class="login-title">Profile Changes Saved</div>';

      if ($profile['user_email'] != $_POST['useremail']) {

        // subject
        $subject = $rdf_name_long." - Email Address Activation";

        // message
        $message = $rdf_name_long." - Email Address Activation\n\n\nYou have requested an email address change for your account on ".$rdf_name.". The next step in order to enable the new email address for the account is to activate it by using the hyperlink below.\n\n\nCurrent E-Mail Address: ".$profile['user_email']."\nNew E-Mail Address: ".$_POST['useremail']."\n\nActivation-Hyperlink: ".$roscms_SET_path_ex."my/activate/".$account_act_code."/\n\n\nBest regards,\nThe ".$rdf_name." Team\n\n\n(please do not reply as this is an auto generated email!)";

        // send the mail
        if (EMail::send($_POST['useremail'], $subject, $message)) {
          echo '<div>Check your email inbox (and spam folder) for the <strong>email-address activation email</strong> that contains the activation hyperlink.</div>';
        }
        else {
          echo '<div style="color: red;">Error while sending E-Mail.</div>';
        }
      }

      if ($password_change) {
        echo '<div>Password changed.</div>';
      }

      echo '<div><a href="'.$roscms_SET_path_ex.'my/" style="color:#FF0000 !important; text-decoration:underline;">My Profile</a></div>';

      ROSUser::syncSubsystems($profile['user_id']);

      unset($_SESSION['rdf_security_code']);
    }
    elseif ($activation_code != '' && strlen($activation_code) > 6) {
      echo_strip('
        <div class="login-title">Activate E-Mail Address</div>
        <div class="login-form">
          <label for="useremail"'.(isset($_POST['registerpost']) ? ' style="color:#FF0000;"' : '').'>New E-Mail Address</label>
          <input name="useremail" type="text" class="input" tabindex="4" id="useremail" value="" size="50" maxlength="50" />
        </div>
        <div class="login-button">
          <input type="submit" name="submit" value="Save" tabindex="16" />
          <br />
          <input type="button" onclick="'."window.location='".$roscms_SET_path_ex."'".' tabindex="17" value="Cancel" name="cancel" style="color:#777777;" />
          <input name="registerpost" type="hidden" id="registerpost" value="reg" />
        </div>');
    }
    else {
      echo_strip('
        <div class="login-title">Edit My Profile</div>
        <div style="font-style:italic">* not required</div>
        <div class="login-form">
          <label for="username">Username</label>
          <input name="username" type="text" class="input" id="username" value="'.$profile['user_name'].'" size="50" maxlength="50" disabled="disabled" value="Klemens Friedl" />
          <br />
          <span style="color:#817A71;">You cannot change your username.</span>
v>
        
        <div class="login-form">
          <label for="userpwd3"'.((isset($_POST['registerpost']) && isset($_POST['userpwd3']) && $_POST['userpwd3'] != '') ? ' style="color:red;"' : '').'>Current Password *</label>
          <input name="userpwd3" type="password" class="input" tabindex="1" id="userpwd3" size="50" maxlength="50" />
          <br />
          <span style="color:#817A71;">Only fill out the three password fields if you want to change your account password!</span>
        </div>

        <div class="login-form">
          <label for="userpwd1"'.((isset($_POST['registerpost']) && isset($_POST['userpwd3']) && $_POST['userpwd3'] != '') ? ' style="color:red;"' : '').'>New Password *</label>
          <input name="userpwd1" type="password" class="input" tabindex="2" id="userpwd1" size="50" maxlength="50" />');
          
      if ($safepwd === false || (isset($_POST['userpwd1']) && strlen($_POST['userpwd1']) > $rdf_register_user_name_max)) {
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
          <label for="userpwd2"'.((isset($_POST['registerpost']) && isset($_POST['userpwd3']) && $_POST['userpwd3'] != '') ? ' style="color:red;"' : '').'>Re-type New Password *</label>
          <input name="userpwd2" type="password" class="input" tabindex="3" id="userpwd2" size="50" maxlength="50" />
        </div>

        <div class="login-form">
          <label for="useremail"'.((isset($_POST['registerpost']) && ($_POST['useremail'] == '' || !EMail::isValid($_POST['useremail']) && $_POST['useremail'] != $profile['user_email'])) ? ' style="color:red"' : '').'>E-Mail</label>
          <input name="useremail" type="text" class="input" tabindex="4" id="useremail" value="'.((isset($_POST['useremail']) && $_POST['useremail'] != '') ?$_POST['useremail'] : $profile['user_email']).'" size="50" maxlength="50" />
          <br />
          <span style="color:#817A71;">Changing the email address involves an activation process.</span>');

      if (isset($_POST['registerpost']) && $existemail && $_POST['useremail'] != $profile['user_email']) {
        echo_strip('
          <br />
          <em>That email address is already with an account. Do you have several accounts? Please <a href="'.$roscms_SET_path_ex.'login/" style="color:red !important; text-decoration:underline;"><strong>login</strong></a>!</em>');
      }

      echo_strip('
        </div>

        <div class="login-form">
          <label for="userfullname">First and Last Name *</label>
          <input name="userfullname" type="text" class="input" tabindex="5" id="userfullname" value="'.(isset($_POST['userfullname']) ? $_POST['userfullname'] : $profile['user_fullname']).'" size="50" maxlength="50" />
        </div>

        <div class="login-form">
          <label for="country"'.(isset($_POST['registerpost']) && $_POST['country'] == '' ? ' style="color:red;"' : '').'>Country</label>
          <select id="country" name="country" tabindex="6">
          <option value="">Select One</option>');

      $stmt=DBConnection::getInstance()->prepare("SELECT coun_id, coun_name FROM user_countries ORDER BY coun_name ASC");
      $stmt->execute();
      while ($country = $stmt->fetch(PDO::FETCH_ASSOC)) {
        echo '<option value="'.$country['coun_id'].'"';

        if ((isset($_POST['country']) && $_POST['country'] == $country['coun_id']) || (empty($_POST['country']) && $country['coun_id'] == $profile['user_country'])) {
          echo ' selected="selected"'; 
        }

        echo '>'.$country['coun_name'].'</option>';
      }

      echo_strip('
          </select>
        </div>
        
        <div class="login-form">
          <label for="tzone"'.(isset($_POST['registerpost']) && isset($_POST['tzone']) && $_POST['tzone'] == '' ? ' style="color:red;"' : '').'>Timezone</label>
          <select name="tzone" id="tzone" tabindex="7">');

      $stmt=DBConnection::getInstance()->prepare("SELECT tz_code, tz_name, tz_value2 FROM user_timezone ORDER BY tz_value ASC");
      $stmt->execute();
      while ($timezone = $stmt->fetch(PDO::FETCH_ASSOC)) {
        echo '<option value="'.$timezone['tz_code'].'"';

        if ((isset($_POST['tzone']) && $_POST['tzone'] == $timezone['tz_code']) || (empty($_POST['tzone']) && $timezone['tz_code'] == $profile['user_timezone'])) {
          echo ' selected="selected"'; 
        }

        echo '>'.$timezone['tz_value2'].' '.$timezone['tz_name'].'</option>';
      }

      echo_strip('
          </select>
        </div>

        <div class="login-form">
          <label for="userwebsite">Private Website *</label>
          <input name="userwebsite" type="text" class="input" tabindex="8" id="userwebsite" value="'.((isset($_POST['userwebsite']) && $_POST['userwebsite'] != '') ? $_POST['userwebsite'] : $profile['user_website']).'" size="50" maxlength="50" />
        </div>

        <div class="login-form">
          <label for="useroccupation">Occupation *</label>
          <input name="useroccupation" type="text" class="input" tabindex="9" id="useroccupation" value="'.((isset($_POST['useroccupation']) && $_POST['useroccupation'] != '') ? $_POST['useroccupation'] : $profile['user_occupation']).'" size="50" maxlength="50" />
        </div>
        <div class="login-form">
          <label for="useroccupation">Location *</label>
          <a href="'.$roscms_intern_webserver_pages.'peoplemap/" target="_blank" style="color:#333333 !important; text-decoration:underline; font-weight:bold;">My Location on the Map</a>
        </div>

        <div class="login-options">
          <fieldset>
            <legend style="color:#817A71;margin-bottom: 10px;">&nbsp;Login Settings&nbsp;</legend>
            <input name="loginoption1" style="width:auto;" type="checkbox" id="loginoption1" value="true"'.(isset($_POST['loginoption1']) || (empty($_POST['registerpost']) && $profile['user_setting_multisession'] == 'true') ? 'checked' : '').' tabindex="11" />
            <label style="display:inline;" for="loginoption1">Multisession</label>
            <br />
            <input name="loginoption2" style="width:auto;" type="checkbox" id="loginoption2" value="true"'.(isset($_POST['loginoption2']) || (empty($_POST['registerpost']) && $profile['user_setting_browseragent'] == 'true') ? 'checked' : '').' tabindex="12" /> 
            <label style="display:inline;" for="loginoption2">Browser Agent Check</label>
            <br />
            <input name="loginoption3" style="width:auto;" type="checkbox" id="loginoption3" value="true"'.((isset($_POST['loginoption3']) || (empty($_POST['registerpost']) && $profile['user_setting_ipaddress'] == 'true')) ? 'checked' : '').' tabindex="13" /> 
            <label style="display:inline;" for="loginoption3">IP Address Check</label>
            <br />
            <input name="loginoption4" style="width:auto;" type="checkbox" id="loginoption4" value="true"'.((isset($_POST['loginoption4']) || (empty($_POST['registerpost']) && $profile['user_setting_timeout'] == 'true')) ? 'checked' : '').' tabindex="14" /> 
            <label style="display:inline;" for="loginoption4">Log me on automatically</label>
          </fieldset>
        </div>

        <div class="login-button">
          <input type="submit" name="submit" value="Save" tabindex="16" />
          <input type="button" onclick="'.("window.location='".$roscms_SET_path_ex."'").'" tabindex="17" value="Cancel" name="cancel" style="color:#777777;" />
          <input name="registerpost" type="hidden" id="registerpost" value="reg" />
        </div>');
    }
    echo_strip('
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </form>');
  }


  private function checkEmailUpdate ( $activation_code, $valid_code)
  {
    return ($activation_code != '' && strlen($activation_code) > 6  // valid activation code
      && isset($_POST['registerpost'])
      && isset($_POST['useremail']) && EMail::isValid($_POST['useremail']) // valdid E-Mail adress
      && $valid_code == $_POST['useremail'].$activation_code); // is given code valid
  }

  
  private function checkPasswordUpdate ( )
  {
    global $rdf_register_user_pwd_min;
    global $rdf_register_user_pwd_max;
  
    return (isset($_POST['registerpost'])
      && isset($_POST['userpwd3']) && $_POST['userpwd3'] != ''
      && isset($_POST['userpwd1']) && $_POST['userpwd1'] != ''
      && isset($_POST['userpwd2']) && $_POST['userpwd2'] != ''
      && strlen($_POST['userpwd1']) >= $rdf_register_user_pwd_min
      && strlen($_POST['userpwd1']) < $rdf_register_user_pwd_max
      && $_POST['userpwd1'] == $_POST['userpwd2']);
  }


} // end of HTML_User_Profile
?>
