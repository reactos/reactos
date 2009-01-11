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
 * class Export_QuickInfo
 * 
 */
class Export_QuickInfo extends Export
{


  public function __construct()
  {
    parent::__construct();

    // check if everything was given, we need to get the quick info
    if (!isset($_GET['d_id']) || !isset($_GET['d_r_id'])) {
      die('No entry was found.');
    }
    $this->getInfo();
  }

  /**
   * Gives an overview about information to the requested data
   *
   * @return 
   * @access private
   */
  private function getInfo( )
  {
    // get current revision
    $stmt=&DBConnection::getInstance()->prepare("SELECT u.name AS user_name, l.name AS language, r.data_id, d.name, d.type, a.name AS acl, r.id, r.version, datetime FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_USERS." u ON r.user_id=u.id JOIN ".ROSCMST_LANGUAGES." l ON l.id=r.lang_id JOIN ".ROSCMST_ACCESS." a ON a.id=d.acl_id WHERE r.id = :rev_id LIMIT 1");
    $stmt->bindParam('rev_id',$_GET['d_r_id'],PDO::PARAM_INT);
    $stmt->execute();
    $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // abort if no entry was found
    if ($revision === false) {
      die('No entry was found.');
    }

    // helper vars
    $t_s = '<span style="color:#FF6600;">'; // tag start
    $t_e = ':</span> '; // tag end
    $t_lb = '<br />'; // tag seperation (linebreak)

    // start to echo metadata
    echo $t_s.'Name'.$t_e . wordwrap($revision['name'],14,"<br />\n",1).$t_lb;
    echo $t_s.'Type'.$t_e . $revision['type'].$t_lb;
    echo $t_s.'Version'.$t_e . $revision['version'].$t_lb;
    echo $t_s.'Lang'.$t_e . $revision['language'].$t_lb;
    echo $t_s.'User'.$t_e . wordwrap($revision['user_name'],13,"<br />\n",1).$t_lb;
    
    // list Tags
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, value  FROM ".ROSCMST_TAGS." WHERE user_id IN(-1, 0, :user_id) AND rev_id=:rev_id ORDER BY user_id ASC, name ASC");
    $stmt->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    $stmt->execute();
    while ($tag = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo $t_s.ucfirst($tag['name']).$t_e . $tag['value'].$t_lb;
    }

    // show additional data for security level > 1
    if (ThisUser::getInstance()->hasAccess('entry_details')) {
      echo $t_s.'Rev-ID'.$t_e.$revision['id'].$t_lb;
      echo $t_s.'Data-ID'.$t_e.$revision['data_id'].$t_lb;
      echo $t_s.'ACL'.$t_e.$revision['acl'].$t_lb;
    }

    // and current Data / Time
    echo $t_s.'Date / Time'.$t_e.$t_lb.$revision['datetime'];
  } // end of member function getInfo


} // end of Export_QuickInfo
?>
