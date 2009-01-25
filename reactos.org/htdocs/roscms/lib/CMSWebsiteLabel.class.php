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
 * class CMSWebsiteLabel
 * 
 */
class CMSWebsiteLabel
{


  public function __construct(  )
  {
    Login::required();

    $this->show();
  } // end of member function __construct


  /**
   * cares about filter management: adding, deleting, listing
   *
   * @access private
   */
  private function show( )
  {
    $thisuser = &ThisUser::getInstance();

    // echo current list of filters
    $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT value FROM ".ROSCMST_TAGS." WHERE user_id = :user_id AND name = 'tag' ORDER BY value ASC");
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute();

    $had_output = false;
    while ($tag = $stmt->fetch(PDO::FETCH_ASSOC)) {
      if($had_output === true) {
        echo ', ';
      }
      echo '<span style="cursor:pointer; text-decoration:underline;wrap:no-wrap;" onclick="'."selectUserTag('".$tag['value']."')".'">'.$tag['value'].'</span>';
      $had_output = true;
    }

    // give standard text, if no label was found
    if ($had_output === false) {
      echo '<span>Tag entries with custom labels to organize the data as it makes sense to you.</span>';
    }
  } // end of member function manage



} // end of CMSWebsiteFilter
?>
