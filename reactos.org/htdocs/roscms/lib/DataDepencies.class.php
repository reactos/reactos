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
  private $short = array('template'=>'templ','content'=>'cont','script'=>'include','page'=>'link','dynamic'=>'link');



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

    $stmt=&DBConnection::getInstance()->prepare("SELECT t.content, t.rev_id FROM ".ROSCMST_TEXT." t JOIN ".ROSCMST_REVISIONS." r ON r.id=t.rev_id WHERE r.archive IS FALSE");
    $stmt->execute();
    while ($text = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $this->rev_id = $text['rev_id'];
      preg_replace_callback('/\[#((inc|templ|cont|link)_([^][#[:space:]]+))\]/', array($this,'newDepency'), $text['content']);
    }
    echo '<strong>Done</strong>';
  }



  /**
   *
   *
   * @access public
   */
  public function addRevision( $rev_id )
  {
    $this->rev_id = $rev_id;

    // get text and parse it for depencies
    $stmt=&DBConnection::getInstance()->prepare("SELECT content, rev_id FROM ".ROSCMST_TEXT." WHERE rev_id=:rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while ($text = $stmt->fetch(PDO::FETCH_ASSOC)) {
      preg_replace_callback('/\[#((inc|templ|cont|link)_([^][#[:space:]]+))\]/', array($this,'newDepency'), $text['content']);
    }
  }



  /**
   *
   *
   * @access public
   */
  public static function renameEntry( $from_name, $to_name, $from_type, $to_type )
  {
    //@TODO
  }



  /**
   *
   *
   * @access public
   */
  public static function removeEntry( $data_id )
  {
    // remove old depencies
    $stmt=&DBConnection::getInstance()->exec("SELECT name, type FROM ".ROSCMST_ENTRIES." WHERE id=:data_id");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->execute();
    $data = $stmt->fetchOnce();
    
    $stmt=&DBConnection::getInstance()->exec("UPDATE ".ROSCMST_DEPENCIES." SET child_id = NULL, child_name = :depency_name WHERE child_id=:data_id");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindValue('depency_name',$this->short[$data['type']].'_'.$data['name']);
    $stmt->execute();
  }



  /**
   *
   *
   * @access public
   */
  public static function removeRevision( $rev_id )
  {
    // remove old depencies
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_DEPENCIES." WHERE reV_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
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
    }

    // try to get depency id
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :name AND type = :type LIMIT 1");
    $stmt->bindParam('name',$matches[3],PDO::PARAM_STR);
    $stmt->bindParam('type',$type,PDO::PARAM_STR);
    $stmt->execute();
    $data_id = $stmt->fetchColumn();

    // insert depency
    if ($data_id === false) {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_DEPENCIES." (rev_id, child_name, include) VALUES (:rev_id, :depency_name, :is_include)");
      $stmt->bindParam('depency_name',$matches[1],PDO::PARAM_STR);
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_DEPENCIES." (rev_id, child_id, include) VALUES (:rev_id, :depency_id, :is_include)");
      $stmt->bindParam('depency_id',$data_id,PDO::PARAM_INT);
    }
    $stmt->bindParam('rev_id',$this->rev_id,PDO::PARAM_INT);
    $stmt->bindParam('is_include',$include,PDO::PARAM_BOOL);
    $stmt->execute();

    return;
  } // end of member function generate_content



} // end of Data
?>
