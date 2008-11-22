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
 * class Export_Maintain
 * 
 */
class Export_Maintain extends Export
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/


  /**
   *
   * @return 
   * @access public
   */
  public function __construct( )
  {
    parent::__construct();
    $this->maintain();
  }


  /**
   *
   * @return 
   * @access public
   */
  public function maintain( )
  {
    global $RosCMS_GET_d_use;
    global $RosCMS_GET_d_value;
    global $RosCMS_GET_d_value2;
    global $RosCMS_GET_d_value3;
    global $RosCMS_GET_d_value4;

    $action = $RosCMS_GET_d_use;

    switch ($action) {
      case 'optimize':
        $stmt=DBConnection::getInstance()->prepare("OPTIMIZE TABLE data_, data_a, data_revision, data_revision_a, data_security, data_stext, data_stext_a, data_tag, data_tag_a, data_tag_name, data_tag_name_a, data_tag_value, data_tag_value_a, data_text, data_text_a, data_user_filter, languages, subsys_mappings, usergroups, usergroup_members, users, user_sessions");
        $stmt->execute();
        Log::writeHigh('optimize database tables: done by '.ThisUser::getInstance()->id().' {data_maintain_out}');
        break;

      case 'analyze':
        $stmt=DBConnection::getInstance()->exec("ANALYZE TABLE data_, data_a, data_revision, data_revision_a, data_security, data_stext, data_stext_a, data_tag, data_tag_a, data_tag_name, data_tag_name_a, data_tag_value, data_tag_value_a, data_text, data_text_a, data_user_filter, languages, subsys_mappings, usergroups, usergroup_members, users, user_sessions");
        $stmt->execute();
        Log::writeHigh('analyze database tables: done by '.ThisUser::getInstance()->id().' {data_maintain_out}');
        break;

      case 'genpages':
        $gentimeA = explode(' ',microtime()); 
        $gentimeA = $gentimeA[1] + $gentimeA[0]; 

        Log::writeGenerateHigh("generate all pages - start");

        $export_html = new Export_HTML();
        echo $export_html->generateFiles("index", "all", "", "all");

        $gentimeB = explode(' ',microtime()); 
        $gentimeB = $gentimeB[1] + $gentimeB[0]; 

        $totaltimef = ($gentimeB - $gentimeA); 
        $showtimef = number_format($totaltimef, 4, '.', ''); 

        Log::writeGenerateHigh('generate all pages - end: '.$showtimef.' seconds');
        break;

      case 'pubdate':
        echo '<h4>Generate related pages of '.$RosCMS_GET_d_value.' ('.$RosCMS_GET_d_value2.', '.$RosCMS_GET_d_value3.', '.$RosCMS_GET_d_value4.')</h4>';

        $stmt=DBConnection::getInstance()->prepare("SELECT data_id FROM data_ WHERE data_name = :name AND data_type = :type LIMIT 1");
        $stmt->bindParam('name', $RosCMS_GET_d_value,PDO::PARAM_STR);
        $stmt->bindParam('type', $RosCMS_GET_d_value2,PDO::PARAM_STR);
        $stmt->execute();
        $data_id = $stmt->fetchColumn();

        echo $export_html->generate($data_id, $RosCMS_GET_d_value3, $RosCMS_GET_d_value4);
        break;
    } // end switch
  }


} // end of Export_Maintain
?>
