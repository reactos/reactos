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
    global $roscms_intern_account_id;
    global $roscms_security_level;

    global $h_a;
    global $h_a2;

    // get current revision
    $stmt=DBConnection::getInstance()->prepare("SELECT (SELECT user_name FROM users WHERE user_id = r.rev_usrid LIMIT 1) AS user_name, (SELECT lang_name FROM languages WHERE lang_id = r.rev_language LIMIT 1) AS lang_name, d.data_id, d.data_name, d.data_type, d.data_acl, r.rev_id, r.rev_version, r.rev_date, r.rev_time FROM data_".$h_a2." d JOIN data_revision".$h_a." r ON r.data_id = d.data_id WHERE r.rev_id = :rev_id AND r.data_id = :data_id ORDER BY r.rev_version DESC LIMIT 1");
    $stmt->bindParam('data_id',$_GET['d_id'],PDO::PARAM_INT);
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
    echo $t_s.'Name'.$t_e . wordwrap($revision['data_name'],14,"<br />\n",1).$t_lb;
    echo $t_s.'Type'.$t_e . $revision['data_type'].$t_lb;
    echo $t_s.'Version'.$t_e . $revision['rev_version'].$t_lb;
    echo $t_s.'Lang'.$t_e . $revision['lang_name'].$t_lb;
    echo $t_s.'User'.$t_e . wordwrap($revision['user_name'],13,"<br />\n",1).$t_lb;
    
    // list Tags
    $stmt=DBConnection::getInstance()->prepare("SELECT n.tn_name, v.tv_value  FROM data_tag".$h_a." a JOIN data_".$h_a2." d ON a.data_id = d.data_id JOIN data_revision".$h_a." r ON a.data_rev_id = r.rev_id JOIN data_tag_name".$h_a." n ON a.tag_name_id = n.tn_id JOIN data_tag_value".$h_a." v ON a.tag_value_id  = v.tv_id WHERE a.data_id IN(0, :data_id) AND a.data_rev_id IN(0, :rev_id) AND a.tag_usrid IN(-1, 0, :user_id) ORDER BY tag_usrid ASC, tn_name ASC");
    $stmt->bindParam('data_id',$revision['data_id'],PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$revision['rev_id'],PDO::PARAM_INT);
    $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
    $stmt->execute();
    while ($tag = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo $t_s.ucfirst($tag['tn_name']).$t_e . $tag['tv_value'].$t_lb;
    }

    // show additional data for security level > 1
    if ($roscms_security_level > 1) {
      echo $t_s.'Rev-ID'.$t_e.$revision['rev_id'].$t_lb;
      echo $t_s.'Data-ID'.$t_e.$revision['data_id'].$t_lb;
      echo $t_s.'ACL'.$t_e.$revision['data_acl'].$t_lb;
    }

    // and current Data / Time
    echo $t_s.'Date / Time'.$t_e.$t_lb.$revision['rev_date'].$t_lb.$revision['rev_time'];
  } // end of member function getInfo


} // end of Export_QuickInfo
?>
