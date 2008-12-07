<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2008  Danny Götte <dangerground@web.de>

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


global $roscms_subsystem_wiki_path;
$roscms_subsystem_wiki_path = '/reactos/wiki/index.php/'; // base adress for wiki pages

/**
 * class RosCMS
 * 
 * contains system specific settings
 */
class RosCMS
{

  // DO NOT MODIFY ANYTHING HERE (except you know what you're doing)
  private $email_support = 'support at reactos.org'; // email to which users can send mails, if they got a problem
  private $email_system = 'ReactOS<noreply@reactos.org>'; // mails which are send from the system and don't require a reply
  
  private $cookie_user_key = 'roscmsusrkey'; // session key
  private $cookie_user_name = 'roscmsusrname'; // user_name
  private $cookie_password = 'rospassword';  // user_password (used for keep login function)
  private $cookie_login_name = 'roscmslogon'; // where username is stored for 'save username' in login options
  private $cookie_security = 'roscmsseckey'; // stores security settings

  private $site_name = 'ReactOS.org'; // sites name
  private $site_language = 'en'; // standard language
  private $site_timezone = -2; // time difference to utc time from server time

  private $path_generated = '/reactos/'; // path to generated files
  private $path_roscms = '/reactos/roscms/'; // path to roscms files

  // system vars
  private $limit_username_min = 4;
  private $limit_username_max = 20;
  private $limit_password_min = 5;
  private $limit_password_max = 50;

  private $system_brand = 'RosCMS v4';
  private $system_version = '4.0.0 alpha';


  /**
   * returns an static instance
   *
   * @access public
   */
  public static function getInstance() {
    static $instance;

    if (empty($instance)) {
      $instance = new RosCMS();
    }
    return $instance;
  }


  /**
   * getter functions
   */
  public function emailSupport() { return $this->email_support; }
  public function emailSystem() { return $this-email_system; }
  public function cookieUserKey() { return $this->cookie_user_key; }

  public function cookieUserName() { return $this->cookie_user_name; }
  public function cookiePassword() { return $this->cookie_password; }
  public function cookieLoginName() { return $this->cookie_login_name; }
  public function cookieSecure() { return $this->cookie_security; }

  public function limitUserNameMin() { return $this->limit_username_min; }
  public function limitUserNameMax() { return $this->limit_username_max; }
  public function limitPasswordMin() { return $this->limit_password_min; }
  public function limitPasswordMax() { return $this->limit_password_max; }

  public function systemBrand( ) { return $this->system_brand; }
  public function systemVersion() { return $this->system_version; }

  public function siteName() { return $this->site_name; }
  public function siteLanguage() { return $this->site_language; }
  public function siteTimezone(){ return $this->site_timezone; }

  public function pathGenerated() { return $this->path_generated; }
  public function pathRosCMS() { return $this->path_roscms; }



  /**
   * setter functions
   */
  public function setEmailSupport( $new_value ){
    $this->email_support = $new_value;
  }

  public function setEmailSystem( $new_value ) {
    $this->email_system = $new_value;
  }

  public function setCookieUserKey( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->cookie_user_key = $new_value;
    else die('bad user key cookie name');
  }

  public function setCookieUserName( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->cookie_user_name = $new_value;
    else die('bad user name cookie name');
  }

  public function setCookiePassword( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->cookie_password = $new_value;
    else die('bad password cookie name');
  }

  public function setCookieLoginName( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->cookie_login_name = $new_value;
    else die('bad login name cookie name');
  }

  public function setCookieSecure( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->cookie_security = $new_value;
    else die('bad security login cookie name');
  }

  public function setSiteName( $new_value ) {
    $this->site_name = $new_value;
  }

  public function setSiteLanguage( $new_value ) {
    $this->site_language = $new_value;
  }

  public function setSiteTimezone( $new_value ) {
    $this->site_timezone = intval($new_value);
  }

  public function setPathGenerated( $new_value ) {
    $this->path_generated = $new_value;
  }
  public function setPathRosCMS( $new_value ) {
    $this->path_roscms = $new_value;
  }


} // end of ThisUser
?>
