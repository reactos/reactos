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
 * class DataDepencies
 * 
 */
class DataDepencies
{

  private $rev_id = 0;



  /**
   *
   *
   * @access public
   */
  public function rebuildDepencies()
  {
    // try to force unlimited script runtime
    @set_time_limit(0);
  
    // remove old depencies
    DBConnection::getInstance()->exec("DELETE FROM ".ROSCMST_DEPENCIES);

    $stmt=&DBConnection::getInstance()->prepare("SELECT content, rev_id FROM ".ROSCMST_TEXT." WHERE name = 'content'");
    $stmt->execute();
    while ($text = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $this->rev_id = $text['rev_id'];
      preg_replace_callback('/\[#(([a-z]+)_([^][#[:space:]]+))\]/', array($this,'newDepency'), $text['content']);
    }
  }




  /**
   *
   * @param string[] matches
   * @return 
   * @access private
   */
  private function newDepency( $matches )
  {

    // get depency type
    switch ($matches[2]) {
      case 'templ':
        $type = 'template';
        $include = true;
        break;
      case 'cont':
        $type = 'content';
        $include = true;
        break;
      case 'inc':
        $type = 'script';
        $include = true;
        break;
      case 'link':
        $type = 'page';
        $include = false;
        break;
      case 'roscms':
        return;
        break;
      default:
        $include = false;
        break;
    }

    // try to get depency id
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :name AND type = :type LIMIT 1");
    $stmt->bindParam('name',$matches[3],PDO::PARAM_STR);
    $stmt->bindParam('type',$type,PDO::PARAM_STR);
    $stmt->execute();
    $data_id = $stmt->fetchColumn();

    // insert depency
    if ($data_id === false) {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_DEPENCIES." (rev_id, depency_name, is_include) VALUES (:rev_id, :depency_name, :is_include)");
      $stmt->bindParam('depency_name',$matches[1],PDO::PARAM_STR);
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_DEPENCIES." (rev_id, depency_id, is_include) VALUES (:rev_id, :depency_id, :is_include)");
      $stmt->bindParam('depency_id',$data_id,PDO::PARAM_INT);
    }
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->bindParam('is_include',$include,PDO::PARAM_BOOL);
    $stmt->execute();

    return;
  } // end of member function generate_content



} // end of Data
?>
