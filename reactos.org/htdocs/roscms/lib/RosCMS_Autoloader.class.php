<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2008-2009 Danny Götte <dangerground@web.de>

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
if (!defined('ROSCMS_PATH')){
  die ('ERROR: unknown RosCMS path');
}

/**
 * class RosCMS_Autoloader
 * 
 */
class RosCMS_Autoloader
{



  /**
   * tries to load a class, by loading the lib/*.class.php file, that should contain the class
   *
   * @param string class name of class, which should be loaded
   * @access public
   */
  public static function autoload( $class )
  {
    if (file_exists(ROSCMS_PATH.'lib/'.$class.'.class.php')) {
      require_once(ROSCMS_PATH.'lib/'.$class.'.class.php');
    }
    elseif (file_exists(ROSCMS_PATH.'lib/om/'.$class.'.class.php')) {
      require_once(ROSCMS_PATH.'lib/om/'.$class.'.class.php');
    }
    elseif (file_exists(ROSCMS_PATH.'lib/view/'.$class.'.class.php')) {
      require_once(ROSCMS_PATH.'lib/view/'.$class.'.class.php');
    }
    elseif (file_exists(ROSCMS_PATH.'lib/backend/'.$class.'.class.php')) {
      require_once(ROSCMS_PATH.'lib/backend/'.$class.'.class.php');
    }
    elseif (file_exists(ROSCMS_PATH.'lib/subsystem/'.$class.'.class.php')) {
      require_once(ROSCMS_PATH.'lib/subsystem/'.$class.'.class.php');
    }
  } // end of member function autoload



} // end of RosCMS_Autoloader



// do something to load RosCMS specific classes
if (function_exists( 'spl_autoload_register' ) ) {

  // try to add a new autoloader class
  spl_autoload_register( array( 'RosCMS_Autoloader', 'autoload' ) );
}
elseif (!function_exists('__autoload')) {

  // try to set an autoload function
  function __autoload($class) {
    RosCMS_Autoloader::autoload($class);
  }
}

// load all needed classes for subsystems staticly
else {

  // Database
  require_once(ROSCMS_PATH.'lib/PDO.class.php');
  require_once(ROSCMS_PATH.'lib/DBConnection.class.php');
  require_once(ROSCMS_PATH.'lib/PDOStatement.class.php');
  require_once(ROSCMS_PATH.'lib/DBStatement.class.php');

  // needed by subsystems
  require_once(ROSCMS_PATH.'lib/om/ROSUser.class.php');
  require_once(ROSCMS_PATH.'lib/om/Cookie.class.php');
  require_once(ROSCMS_PATH.'lib/om/Login.class.php');

  // Subsystem related
  require_once(ROSCMS_PATH.'lib/Subsystem.class.php');
  require_once(ROSCMS_PATH.'lib/subsys/Wiki.class.php');
  require_once(ROSCMS_PATH.'lib/subsys/PHPBB.class.php');
  // bugzilla cares itself about login, so we don't need to include something for it
}

?>
