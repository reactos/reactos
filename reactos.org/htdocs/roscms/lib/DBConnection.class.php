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
 * class DBConnection
 * 
 */
class DBConnection extends PDO
{



  /**
   * connects to database
   *
   * @return object
   * @access public
   */
  public function __construct()
  {
    // load database authentification config
    require_once(ROSCMS_PATH.'connect.db.php');

    try {
      parent::__construct('mysql:dbname='.$db_name.';host='.$db_host.';port='.$db_port, $db_user, $db_pass);

      // unset loaded db config
      unset($db_name);
      unset($db_host);
      unset($db_user);
      unset($db_pass);
      unset($db_port);

      // show errors as warning, and use our own statement class
      $this->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
      $this->setAttribute(PDO::ATTR_STATEMENT_CLASS,array('DBStatement', array($this)));
    }
    catch (PDOException $e) {

      die('<div>Connection failed: <span style="color:red;">'.$e->getMessage().'</span></div>');
    }
  } // end of constructor



  /**
   * returns the instance to our DB Object, so we can call it every time, without needing a variable to it
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
  } // end of member function getInstance



} // end of DBConnection
?>
