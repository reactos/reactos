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
 * class Language
 * 
 */
class DBStatement extends PDOStatement
{
    public $dbh;


    protected function __construct($handle) {
        $this->dbh = $handle;
    }


  /**
   * bypass unbuffered query problem with mysql drivers
   *
   * @param string fetch_mode is one of the PDO::FETCH_* constants

   * @return 
   * @access public
   */
  public function fetchOnce( $fetch_mode = null )
  {
    
    if ($fetch_mode == null) $result = parent::fetch();
    else $result = parent::fetch($fetch_mode);
    parent::closeCursor();
    return $result;
  } // end of member function check_lang
  
  public function fetchColumn( $offset = 0)
  {
    
    $result = parent::fetchColumn( $offset );
    parent::closeCursor();
    return $result;
  } // end of member function check_lang

} // end of Language
?>
