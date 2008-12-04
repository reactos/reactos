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


// RosCMS Table Names
define('ROSCMST_ACCESS'     , 'roscms_access');
define('ROSCMST_USERS'      , 'roscms_accounts');
define('ROSCMST_FORBIDDEN'  , 'roscms_accounts_forbidden');
define('ROSCMST_SESSIONS'   , 'roscms_accounts_sessions');
define('ROSCMST_COUNTRIES'  , 'roscms_countries');
define('ROSCMST_ENTRIES'    , 'roscms_entries');
define('ROSCMST_REVISIONS'  , 'roscms_entries_revisions');
define('ROSCMST_STEXT'      , 'roscms_entries_stext');
define('ROSCMST_TAGS'       , 'roscms_entries_tags');
define('ROSCMST_TEXT'       , 'roscms_entries_text');
define('ROSCMST_FILTER'     , 'roscms_filter');
define('ROSCMST_GROUPS'     , 'roscms_groups');
define('ROSCMST_JOBS'       , 'roscms_jobs');
define('ROSCMST_LANGUAGES'  , 'roscms_languages');
define('ROSCMST_SUBSYS'     , 'roscms_rel_accounts_subsys');
define('ROSCMST_MEMBERSHIPS', 'roscms_rel_groups_accounts');
define('ROSCMST_ACL'        , 'roscms_rel_groups_access');
define('ROSCMST_DEPENCIES'  , 'roscms_rel_revisions_depencies');
define('ROSCMST_TIMEZONES'  , 'roscms_timezones');



/**
 * class Language
 * 
 */
class DBConnection extends PDO
{
  public function __construct()
  {
    include_once(ROSCMS_PATH.'connect.db.php');

    try {
      parent::__construct('mysql:dbname='.DB_NAME.';host='.DB_HOST, DB_USER, DB_PASS);
      
      $this->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
      $this->setAttribute(PDO::ATTR_STATEMENT_CLASS,array('DBStatement', array($this)));
    }
    catch (PDOException $e) {
    
      echo '<div>Connection failed: <span style="color:red;">'.$e->getMessage().'</span></div>';
      //print_debug_backtrace();
      exit();
    }
  }


  /**
   * returns the instance to out DB Object
   *
   * @return object
   * @access public
   */
  public static function getInstance( )
  {
    static $instance;
    
    if (empty($instance)) {
      $instance = new DBConnection();
    }
    
    return $instance;
  } // end of member function check_lang

} // end of DBConnection
?>
