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
 * class Security
 * 
 */
class Security
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/


  /**
   * 
   *
   * @param string kind kind of access is asked for
   * @return ACL
   * @access public
   */
  public function getACL( $kind )
  {
    global $roscms_intern_account_id;
    global $roscms_security_level;

    $acl = '';
    $sec_access = false;  // security access already granted ?

    // only if user has rights to access the interface
    if ($roscms_security_level > 0) {

      // for usage in the while loop
      $stmt=DBConnection::getInstance()->prepare("SELECT usergroupmember_usergroupid FROM usergroup_members WHERE usergroupmember_userid = :user_id");
      $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
      $stmt->execute();
      $usergroups = $stmt->fetchAll(PDO::FETCH_ASSOC);

      // go through acl's
      $stmt=DBConnection::getInstance()->prepare("SELECT sec_name, sec_deny, sec_allow, sec_lev1_read, sec_lev2_read, sec_lev3_read, sec_lev1_write, sec_lev2_write, sec_lev3_write, sec_lev1_add, sec_lev2_add, sec_lev3_add, sec_lev1_pub, sec_lev2_pub, sec_lev3_pub, sec_lev1_trans, sec_lev2_trans, sec_lev3_trans FROM data_security WHERE sec_branch = 'website'");
      $stmt->execute();
      while ($sec_entry = $stmt->fetch(PDO::FETCH_ASSOC)) {
      
        // add entries, remove them if they're on the deny list
        if ($sec_entry['sec_lev'.$roscms_security_level.'_'.$kind] == 1) {
          if ($sec_access) {
            $acl .= " OR";
          }

          $acl .= " data_acl = ".DBConnection::getInstance()->quote($sec_entry['sec_name'])." ";
          $sec_access = true;

          // check for denied groups and remove them if needed
          reset($usergroups);
          foreach($usergroups as $usergroup) {

            // is usergroup on denied list ?
            $pos = strpos($sec_entry['sec_deny'], '|'.$usergroup['usergroupmember_usergroupid'].'|');
            if ($pos !== false) {

              // is usergroup already added to our acl list ?
              $pos = strpos($acl, " data_acl = ".DBConnection::getInstance()->quote($sec_entry['sec_name']));
              if ($pos === false) {
                die('roscms_security_sql(): problem');  //@CHECKME should this really die here ? instead of simply removing the entry
              }
              else {

                // remove denied entries from allowed list, remove leading OR
                $temp_sql = str_replace(" OR data_acl = ".DBConnection::getInstance()->quote($sec_entry['sec_name'])."' ", "", $acl);
                $temp_sql = str_replace(" data_acl = ".DBConnection::getInstance()->quote($sec_entry['sec_name'])."' ", "", $acl);

                // prevent starting with ' OR'
                if ($temp_sql == '') {
                  $sec_access = false;  
                }
              }
            }
          } // foreach
        }
        else {

          // add entries on the allow list, if they aren't set by acl
          reset($usergroups);
          foreach($usergroups as $usergroup) {
          
            // is current usergroup on the allowed list ?
            $pos = strpos($sec_entry['sec_allow'], "|".$usergroup['usergroupmember_usergroupid']."|");
            if ($pos !== false) {

              // add to acl list
              if ($sec_access) {
                $acl .= " OR";
              }
              $acl .= " data_acl = ".DBConnection::getInstance()->quote($sec_entry['sec_name'])." ";
              $sec_access = true;
            }
          } // foreach
        }
      } // while
    }

    // group our acl list, or fail because no rights to access
    if ($sec_access > 0) {
      $acl = " AND (". $acl .") ";
    }
    else {
      $acl = " AND FALSE ";
    }

    return $acl;
  } // end of member function getACL


  /**
   * Constructs a list of things the user can do
   *
   * @param int data_id 
   * @return rights list
   * @access private
   */
  private function getRightsList( $data_id )
  {
    global $h_a2;

    global $roscms_intern_account_id;
    global $roscms_security_level;

    // roscms interface access ?
    if ($roscms_security_level < 1) {
      return;
    }

    $rights_list = '|';
    $acl_allow = false;
    $acl_deny = false;

    // get rights
    $stmt=DBConnection::getInstance()->prepare("SELECT sec_allow, sec_deny, sec_lev1_read, sec_lev2_read, sec_lev3_read, sec_lev1_write, sec_lev2_write, sec_lev3_write, sec_lev1_add, sec_lev2_add, sec_lev3_add, sec_lev1_pub, sec_lev2_pub, sec_lev3_pub, sec_lev1_trans, sec_lev2_trans, sec_lev3_trans FROM data_".$h_a2." d JOIN data_security y ON y.sec_name = d.data_acl WHERE data_id = :data_id AND y.sec_branch = 'website' LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->execute() or die('Data-Entry "'.$data_id.'" not found [usergroups].');
    $rights = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    // check for membership in allowed groups
    $stmt=DBConnection::getInstance()->prepare("SELECT usergroupmember_usergroupid FROM usergroup_members WHERE usergroupmember_userid = :user_id");
    $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
    $stmt->execute();
    while($usergroup = $stmt->fetch(PDO::FETCH_ASSOC)) {

      //
      $pos = strpos($rights['sec_allow'], "|".$usergroup['usergroupmember_usergroupid']."|");
      if ($pos !== false) {
        $acl_allow = true;
      }
    }

    // check for membership in denied list
    $stmt=DBConnection::getInstance()->prepare("SELECT usergroupmember_usergroupid FROM usergroup_members WHERE usergroupmember_userid = :user_id");
    $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
    $stmt->execute();
    while($usergroup = $stmt->fetch(PDO::FETCH_ASSOC)) {

      //
      $pos = strpos($rights['sec_deny'], "|".$usergroup['usergroupmember_usergroupid']."|");
      if ($pos !== false) {
        $acl_deny = true;
      }
    }

    // create a list with rights
    //@CHECKME is this type of checks a good idea ??
    if (($rights['sec_lev'.$roscms_security_level.'_read'] == 1 || $acl_allow === true) && $acl_deny === false) {
      $rights_list .= 'read|';
    }
    if (($rights['sec_lev'.$roscms_security_level.'_write'] == 1 || $acl_allow === true) && $acl_deny === false) {
      $rights_list .= 'write|';
    }
    if (($rights['sec_lev'.$roscms_security_level.'_add'] == 1 || ($acl_allow === true && $roscms_security_level == 3)) && $acl_deny === false) {
      $rights_list .= 'add|';
    }
    if (($rights['sec_lev'.$roscms_security_level.'_pub'] == 1 || ($acl_allow === true && $roscms_security_level == 3)) && $acl_deny === false) {
      $rights_list .= 'pub|';
    }
    if (($rights['sec_lev'.$roscms_security_level.'_trans'] == 1 || ($acl_allow === true && $roscms_security_level == 3)) && $acl_deny === false) {
      $rights_list .= 'trans|';
    }

    return $rights_list;
  } // end of member function getRightsList

  /**
   * checks if the user has the given right to do things
   *
   * @param int data_id 
   * @param string kind kind of rights e.g. 'read' 
   * @return 
   * @access public
   */
  public function hasRight( $data_id, $kind )
  {
    global $roscms_security_level;

    // only if roscms interface access is granted
    if ($roscms_security_level < 1) {
echo $roscms_security_level;
      return false;
    }


    // return if the requested kind of right is in the rights list for the user
    $pos = strpos(self::getRightsList($data_id), '|'.$kind.'|');
    return ($pos !== false);
  } // end of member function hasRight

  /**
   * gives a short overview about user rights
   *
   * @param int data_id 
   * @return explanation
   * @access public
   */
  public function rightsOverview( $data_id )
  {
    global $roscms_security_level;

    // only if roscms interface access is granted
    if ($roscms_security_level < 1) {
      return;
    }

    $rights_list = self::getRightsList($data_id);  // so we don't need to call the same function several times
    $explanation = ''; // contains abbreviations for each right or a - (missing) symbol instead

    // start to construct list
    $explanation .= (strpos($rights_list, '|read|') === false)    ? '-' : 'r';
    $explanation .= (strpos($rights_list, '|write|') === false)   ? '-' : 'w';
    $explanation .= (strpos($rights_list, '|add|') === false)     ? '-' : 'a';
    $explanation .= (strpos($rights_list, '|pub|') === false)     ? '-' : 'p';
    $explanation .= (strpos($rights_list, '|trans|') === false)   ? '-' : 'p';

    // add also security level
    $explanation .= ' '.$roscms_security_level;

    return $explanation;
  } // end of member function rightsOverview


} // end of Security
?>
