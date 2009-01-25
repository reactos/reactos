<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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
 * class CMSWebsiteFilter
 * 
 */
class CMSWebsiteFilter
{


  public function __construct(  )
  {
    Login::required();

    // manage actions for adding / deleting filters
    if ($_GET['d_val'] == 'add') {
      $this->add($_GET['title'], $_GET['setting']);
    }
    elseif ($_GET['d_val'] == 'del') {
      $this->del($_GET['id']);
    }

    // show updated filter list
    $this->show();

  } // end of member function __construct



  /**
   * adds a new filter to users smart filters
   *
   * @param string title the filter name
   * @param string setting filter settings
   * @return 
   * @access private
   */
  private function add( $title, $setting )
  {
    $thisuser = &ThisUser::getInstance();

    // check if filter already exists
    $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_FILTER." WHERE user_id = :user_id AND name = :title LIMIT 1");
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->bindParam('title',$title,PDO::PARAM_STR);
    $stmt->execute();
    if ($stmt->fetchColumn() === false) {

      // insert new filter
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_FILTER." ( id, user_id, name, setting ) VALUES ( NULL, :user_id, :title, :setting )");
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->bindParam('title',$title,PDO::PARAM_STR); 
      $stmt->bindParam('setting',$setting,PDO::PARAM_STR);
      $stmt->execute();
    }
  }



  /**
   * deletes a smart filter
   *
   * @param string _GET['d_val3'] filter_title if adding its the filter name, if del it's the filter id
   * @param string _GET['d_val4'] filter_string filter content
   * @return 
   * @access private
   */
  private function del( $filter_id )
  {
    // delete a label
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_FILTER." WHERE id = :filter_id AND user_id = :user_id LIMIT 1");
    $stmt->bindParam('filter_id',$filter_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    $stmt->execute();
  }



  /**
   * deletes a smart filter
   *
   * @access private
   */
  private function show( )
  {
    // echo current list of filters
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, setting FROM ".ROSCMST_FILTER." WHERE user_id = :user_id ORDER BY name ASC");
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    $stmt->execute();
    while ($filter = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo_strip('
        <span style="cursor:pointer; text-decoration:underline;" onclick="'."selectUserFilter('".$filter['setting']."', '".$filter['name']."')".'">'.$filter['name'].'</span>
        <span style="cursor:pointer;" onclick="'."deleteUserFilter('".$filter['id']."', '".$filter['name']."')".'">
          <img src="images/remove.gif" alt="-" style="width:11px; height:11px; border:0px;" />
        </span>
        <br />');
    }

    // give standard text, if no filters are found
    if ($filter === false) {
      echo '<span>Compose your favorite filter combinations and afterwards use the &quot;save&quot; function.</span>';
    }
  }



} // end of CMSWebsiteFilter
?>
