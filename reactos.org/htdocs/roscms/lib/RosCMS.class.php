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

/**
 * class RosCMS
 * 
 * contains system specific settings
 */
class RosCMS
{

  // DO NOT MODIFY ANYTHING HERE (except you know what you're doing)
  private $email_support = null; // email to which users can send mails, if they got a problem
  private $email_system = null; // mails which are send from the system and don't require a reply
  
  private $cookie_user_key = null; // session key
  private $cookie_user_name = null; // user_name
  private $cookie_password = null;  // user_password (used for keep login function)
  private $cookie_login_name = null; // where username is stored for 'save username' in login options
  private $cookie_security = null; // stores security settings

  private $site_name = null; // sites name
  private $site_language = null; // standard language
  private $site_timezone = null; // time difference to utc time from server time

  private $path_generated = null; // path to generated files
  private $path_generation_cache = null; // path to cache files while generation process
  private $path_roscms = null; // path to roscms files
  private $path_instance = null; // path to current roscms instance

  // system vars
  private $limit_username_min = 4;
  private $limit_username_max = 20;
  private $limit_password_min = 5;
  private $limit_password_max = 50;

  private $system_brand = 'RosCMS 4';
  private $system_version = '4.0.0 alpha';


  private $applied = false;
  private $config = array();


  /**
   * returns an static instance
   *
   * @access public
   */
  public static function getInstance()
  {
    static $instance;

    if (empty($instance)) {
      $instance = new RosCMS();
    }
    return $instance;
  } // end of member function getInstance



  /**
   * apply temporary config data, if not already set
   *
   * @access public
   */
  public function apply()
  {
    
    foreach ($this->config as $key => $val) {
      if ($this->$key === null) $this->$key = $val;
    }
    $this->applied=true;
  } // end of member function apply



  /**
   * registers a new table name, if not already registered
   *
   * @access public
   */
  public function setTable($table, $name)
  {
    if (!defined($table)) {
      define($table, $name);
    }
  } // end of member function setTable



  /**
   * getter functions
   */
  public function emailSupport() { if ($this->applied) return $this->email_support; }
  public function emailSystem() { if ($this->applied) return $this-email_system; }
  public function cookieUserKey() { if ($this->applied) return $this->cookie_user_key; }

  public function cookieUserName() { if ($this->applied) return $this->cookie_user_name; }
  public function cookiePassword() { if ($this->applied) return $this->cookie_password; }
  public function cookieLoginName() { if ($this->applied) return $this->cookie_login_name; }
  public function cookieSecure() { if ($this->applied) return $this->cookie_security; }

  public function limitUserNameMin() { return $this->limit_username_min; }
  public function limitUserNameMax() { return $this->limit_username_max; }
  public function limitPasswordMin() { return $this->limit_password_min; }
  public function limitPasswordMax() { return $this->limit_password_max; }

  public function systemBrand( ) { return $this->system_brand; }
  public function systemVersion() { return $this->system_version; }

  public function siteName() { if ($this->applied) return $this->site_name; }
  public function siteLanguage() { if ($this->applied) return $this->site_language; }
  public function siteTimezone(){ if ($this->applied) return $this->site_timezone; }

  public function pathGenerated() { if ($this->applied) return $this->path_generated; }
  public function pathGenerationCache() { if ($this->applied) return $this->path_generation_cache; }
  public function pathRosCMS() { if ($this->applied) return $this->path_roscms; }
  public function pathInstance() { if ($this->applied) return $this->path_instance; }



  /**
   * setter functions
   */
  public function setEmailSupport( $new_value ){
    $this->config['email_support'] = $new_value;
  }

  public function setEmailSystem( $new_value ) {
    $this->config['email_system'] = $new_value;
  }

  public function setCookieUserKey( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->config['cookie_user_key'] = $new_value;
    else die('bad user key cookie name');
  }

  public function setCookieUserName( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->config['cookie_user_name'] = $new_value;
    else die('bad user name cookie name');
  }

  public function setCookiePassword( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->config['cookie_password'] = $new_value;
    else die('bad password cookie name');
  }

  public function setCookieLoginName( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->config['cookie_login_name'] = $new_value;
    else die('bad login name cookie name');
  }

  public function setCookieSecure( $new_value ) {
    if (preg_match('/[A-Za-z0-9_]+/', $new_value)) $this->config['cookie_security'] = $new_value;
    else die('bad security login cookie name');
  }

  public function setSiteName( $new_value ) {
    $this->config['site_name'] = $new_value;
  }

  public function setSiteLanguage( $new_value ) {
    $this->config['site_language'] = $new_value;
  }

  public function setSiteTimezone( $new_value ) {
    $this->config['site_timezone'] = intval($new_value);
  }

  public function setPathGenerated( $new_value ) {
    $this->config['path_generated'] = $new_value;
  }

  public function setPathGenerationCache( $new_value ) {
    $this->config['path_generation_cache'] = $new_value;
  }

  public function setPathRosCMS( $new_value ) {
    $this->config['path_roscms'] = $new_value;
  }

  public function setPathInstance( $new_value ) {
    $this->config['path_instance'] = $new_value;
  }



} // end of RosCMS
?>
