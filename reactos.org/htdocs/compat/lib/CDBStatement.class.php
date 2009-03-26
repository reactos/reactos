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
 * class CDBStatement
 * 
 */
class CDBStatement extends PDOStatement
{
    protected $pdo;


  /**
   * called by alternate statement function in PDO
   *
   * @param object handle
   * @access private
   */
    private function __construct( $handle ) {
      $this->pdo = $handle;
    }



  /**
   * bypass unbuffered query problem with mysql drivers, as some Webserver seems to have problems with that
   *
   * @param string fetch_mode is one of the PDO::FETCH_* constants
   * @return mixed[]
   * @access public
   */
  public function fetchOnce( $fetch_mode = null )
  {
    // because I've no idea what standard value is used, we'll protected the function from our standard value
    if ($fetch_mode === null) {
      $result = parent::fetch();
    }
    else {
      $result = parent::fetch($fetch_mode);
    }
    
    // thats important, to free our resources
    parent::closeCursor();
    return $result;
  } // end of member function fetchOnce



  /**
   * bypass unbuffered query problem with mysql drivers
   *
   * @param string offset field offset
   * @return mixed
   * @access public
   */
  public function fetchColumnOnce( $offset = 0)
  {
    // use original version
    $result = parent::fetchColumn( $offset );

    // free result
    parent::closeCursor();
    return $result;
  } // end of member function check_lang



} // end of CDBStatement
?>
