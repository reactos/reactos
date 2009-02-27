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
 * @class Dependencies
 *
 * Revisions are dependent from other entries, that means a revision can be dependent from a revision of the same language, or as fallback to any other language, preferable to the standard language
 * if not entry id exists, only a dependency to the name is saved
 * 
 */
class Dependencies
{

  private $rev_id = 0;
  private $short = array('content'=>'cont','script'=>'include','page'=>'link','dynamic'=>'link');



  /**
   * delete the old tree, and build it new
   *
   * @return bool
   * @access public
   */
  public function rebuild( )
  {
    // try to force unlimited script runtime
    @set_time_limit(0);
  
    // remove old dependencies
    DBConnection::getInstance()->exec("DELETE FROM ".ROSCMST_DEPENDENCIES." WHERE user_defined IS FALSE");

    // walk trough all stable
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_REVISIONS." WHERE archive IS FALSE AND status='stable'");
    if ($stmt->execute()) {
      while ($revision = $stmt->fetch(PDO::FETCH_ASSOC)) {

        // add dependencies for current revision
        $this->addRevision($revision['id']);
      }
      return true;
    }
    return false;
  } // end of member function rebuild



  /**
   * adds dependencies for this revision
   *
   * @param int rev_id
   * @return bool
   * @access public
   */
  public function addRevision( $rev_id )
  {
    $this->rev_id = $rev_id;

    // get text and parse it for dependencies
    $stmt=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_TEXT." WHERE rev_id=:rev_id AND name='content'");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    if ($stmt->execute()) {
      return preg_replace_callback('/\[#((inc|templ|cont|link)_([^][#[:space:]]+))\]/', array($this,'newDependency'), $stmt->fetchColumn());
    }
    return false;
  } // end of member function addRevision



  /**
   * @FILLME
   *
   * @param string from_name
   * @param string to_name
   * @param string from_type
   * @param string to_type
   * @return bool
   * @access public
   */
  public static function renameEntry( $from_name, $to_name, $from_type, $to_type )
  {
    //@TODO
  } // end of member function renameEntry



  /**
   * replaces a data_id with the link name, the data would have, if it would exist
   *
   * @param data_id
   * @return bool
   * @access public
   */
  public static function removeEntry( $data_id )
  {
    // get name & type
    $stmt=&DBConnection::getInstance()->exec("SELECT name, type FROM ".ROSCMST_ENTRIES." WHERE id=:data_id");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    if ($stmt->execute()) {
      $data = $stmt->fetchOnce();

      // replace old dependency per id to text dependency
      $stmt=&DBConnection::getInstance()->exec("UPDATE ".ROSCMST_DEPENDENCIES." SET child_id = NULL, child_name = :dependency_name WHERE child_id=:data_id");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->bindValue('dependency_name',$this->short[$data['type']].'_'.$data['name']);
      return $stmt->execute();
    }
    return false;
  } // end of member function removeEntry



  /**
   * removes all dependencies from a revision to all other data_ids
   *
   * @param int rev_id
   * @return bool
   * @access public
   */
  public static function removeRevision( $rev_id )
  {
    // remove old dependencies
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_DEPENDENCIES." WHERE reV_id = :rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    return $stmt->execute();
  } // end of member function removeRevision



  /**
   * add a new user defined dependency
   *
   * @param int rev_id
   * @param string name entry name
   * @param string type entry type
   * @return bool
   * @access public
   */
  public static function addManual( $rev_id, $name, $type )
  {
    // check access rights
    if (ThisUser::getInstance()->hasAccess('add_dependencies')) {
      return self::insert($rev_id, $name, $type, true);
    }
    return false;
  } // end of member function addManual



  /**
   * deletes a user defined dependency
   *
   * @param int dep_id dependency id
   * @return bool
   * @access public
   */
  public static function deleteManual( $dep_id )
  {
    // check access rights
    if (ThisUser::getInstance()->hasAccess('add_dependencies')) {

      // delete dependencies
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_DEPENDENCIES." WHERE id = :dep_id AND user_defined IS TRUE");
      $stmt->bindParam('dep_id',$dep_id,PDO::PARAM_INT);
      return $stmt->execute();
    }
    return false;
  } // end of member function deleteManual



  /**
   * inserts/updates new dependency into database
   *
   * @param string[] matches
   * @return bool
   * @access private
   */
  private function newDependency( $matches )
  {
    // get dependency type for database
    switch ($matches[2]) {
      case 'cont':
        $type = 'content';
        break;
      case 'inc':
        $type = 'script';
        break;
      case 'link':
        $type = 'page';
        break;
    }

    return self::insert($this->rev_id, $matches[3], $type, false);
  } // end of member function newDependency



  /**
   * shared function to insert/update entries
   *
   * @param int rev_id parent
   * @param string name child
   * @param string type child
   * @param bool user_defined
   * @return bool
   * @access public
   */
  private static function insert( $rev_id, $name, $type, $user_defined )
  {
    // is include dependency ?
    switch ($type) {
      case 'content':
        $dependency_name = 'cont_'.$name;
        $include = true;
        break;
      case 'script':
        $dependency_name = 'inc_'.$name;
        $include = true;
        break;
      case 'dynamic':
        $dependency_name = 'link_'.$name;
        $include = true && $user_defined;
        break;
      case 'page':
        $dependency_name = 'link_'.$name;
        $include = false;
        break;
    }

    // check for existing entry
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :name AND type = :type LIMIT 1");
    $stmt->bindParam('name',$name,PDO::PARAM_STR);
    $stmt->bindParam('type',$type,PDO::PARAM_STR);
    if ($stmt->execute()) {
      $data_id = $stmt->fetchColumn();

      // check if we already have an dependency to this entry
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_DEPENDENCIES." WHERE child_name=:dependency_name");
      $stmt->bindParam('dependency_name',$dependency_name,PDO::PARAM_STR);
      $stmt->execute();
      $dependency_id = $stmt->fetchColumn();

      // update entry with data id instead of name&type
      if ($dependency_id !== false) {
        $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_DEPENDENCIES." SET child_name=NULL, child_id=:dependency_id WHERE id=:dependency_id");
        $stmt->bindParam('child_id',$data_id,PDO::PARAM_INT);
        $stmt->bindParam('dependency_id',$dependency_id,PDO::PARAM_INT);
        return $stmt->execute();
      }

      // insert new dependency
      else {

        // insert dependency with name
        if ($data_id === false) {
          $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_DEPENDENCIES." (rev_id, child_name, include, user_defined) VALUES (:rev_id, :dependency_name, :is_include, :user_defined)");
          $stmt->bindParam('dependency_name',$dependency_name,PDO::PARAM_STR);
        }

        // insert dependency with id
        else {
          $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_DEPENDENCIES." (rev_id, child_id, include, user_defined) VALUES (:rev_id, :dependency_id, :is_include, :user_defined)");
          $stmt->bindParam('dependency_id',$data_id,PDO::PARAM_INT);
        }
        $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
        $stmt->bindParam('is_include',$include,PDO::PARAM_BOOL);
        $stmt->bindParam('user_defined',$user_defined,PDO::PARAM_BOOL);
        return $stmt->execute();
      }
    }
    return false;
  } // end of member function deleteManual



} // end of Dependencies
?>
