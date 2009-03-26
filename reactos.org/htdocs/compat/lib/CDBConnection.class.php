<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2008-2009  Danny Gštte <dangerground@web.de>

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
 * class CDBConnection
 * This name is a hack, to not use roscms classes
 * 
 */
class CDBConnection extends PDO
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
    require_once(CDB_PATH.'connect.db.php');

    try {
      parent::__construct('mysql:dbname='.$cdb_name.';host='.$cdb_host.';port='.$cdb_port, $cdb_user, $cdb_pass);

      // unset loaded db config
      unset($cdb_name);
      unset($cdb_host);
      unset($cdb_user);
      unset($cdb_pass);
      unset($cdb_port);

      // show errors as warning, and use our own statement class
      $this->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
      $this->setAttribute(PDO::ATTR_STATEMENT_CLASS,array('CDBStatement', array($this)));
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
      $instance = new CDBConnection();
    }
    
    return $instance;
  } // end of member function getInstance



} // end of CDBConnection
?>
