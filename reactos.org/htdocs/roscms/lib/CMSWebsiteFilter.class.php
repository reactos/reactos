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

  private $type_num = 1;


  public function __construct(  )
  {
    Login::required();

    $this->manage();
  } // end of member function __construct


  /**
   * cares about filter management: adding, deleting, listing
   *
   * @param string _GET['d_value']  action 'add' or 'del'
   * @param string _GET['d_value2'] type 'label' or not
   * @param string _GET['d_value3'] filter_title if adding its the filter name, if del it's the filter id
   * @param string _GET['d_value4'] filter_string filter content
   * @return 
   * @access private
   */
  private function manage( )
  {
    $thisuser = &ThisUser::getInstance();

    // they need some standard values
    $action = (isset($_GET['d_val']) ? $_GET['d_val'] : '');
    $type = (isset($_GET['d_val2']) ? $_GET['d_val2'] : '');
    $filter_title = (isset($_GET['d_val3']) ? $_GET['d_val3'] : '');
    $filter_string = (isset($_GET['d_val4']) ? $_GET['d_val4'] : '');

    if ($type == 'label') {
      $this->type_num = 2; // standard value is set in var declaration
    }

    // add a new label
    if ($action == 'add') {

      // check if filter already exists
      $stmt=DBConnection::getInstance()->prepare("SELECT 1 FROM data_user_filter WHERE filt_usrid = :user_id AND filt_title = :title AND filt_type = :type LIMIT 1");
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->bindParam('title',$filter_title,PDO::PARAM_STR); 
      $stmt->bindParam('type',$this->type_num,PDO::PARAM_INT);
      $stmt->execute();
      if ($stmt->fetchColumn() === false) {

        // insert new filter
        $stmt=DBConnection::getInstance()->prepare("INSERT INTO data_user_filter ( filt_id , filt_usrid , filt_title , filt_type , filt_string , filt_datetime , filt_usage , filt_usagedate ) VALUES ( NULL, :user_id, :title, :type, :string, NOW(), 1, NOW() )");
        $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
        $stmt->bindParam('title',$filter_title,PDO::PARAM_STR); 
        $stmt->bindParam('type',$this->type_num,PDO::PARAM_INT);
        $stmt->bindParam('string',$filter_string,PDO::PARAM_STR);
        $stmt->execute();
      }
    }
    elseif ($action == 'del') {
      // delete a label
      $stmt=DBConnection::getInstance()->prepare("DELETE FROM data_user_filter WHERE filt_id = :filter_id AND filt_usrid = :user_id LIMIT 1");
      $stmt->bindParam('filter_id',$filter_title,PDO::PARAM_INT);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
      $stmt->execute();
    }

    // echo current list of filters
    $stmt=DBConnection::getInstance()->prepare("SELECT filt_id, filt_title, filt_string FROM data_user_filter WHERE filt_usrid = :user_id AND filt_type = :type ORDER BY filt_title ASC");
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->bindParam('type',$this->type_num,PDO::PARAM_INT);
    $stmt->execute();
    while ($filter = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo_strip('
        <span style="cursor:pointer; text-decoration:underline;" onclick="'."selectUserFilter('".$filter['filt_string']."', '".$type."', '".$filter['filt_title']."')".'">'.$filter['filt_title'].'</span>
        <span style="cursor:pointer;" onclick="'."deleteUserFilter('".$filter['filt_id']."', '".$type."', '".$filter['filt_title']."')".'">
          <img src="images/remove.gif" alt="-" style="width:11px; height:11px; border:0px;" />
        </span>
        <br />');
    }

    // give standard text, if no filters are found
    if ($filter === false) {
      if ($type == 'label') {
        echo '<span>Tag entries with custom labels to organize the data as it makes sense to you.</span>';
      }
      else {
        echo '<span>Compose your favorite filter combinations and afterwards use the "save" function.</span>';
      }
    }

    // echo add 'button'
    if ($type == 'label') {
      echo '<div style="cursor: pointer; text-align:right;" onclick="'."addUserFilter('label', '')".'"  style="text-decoration:underline;">Add</div>';
    }
  } // end of member function manage



} // end of CMSWebsiteFilter
?>
