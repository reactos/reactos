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
 * class Export_Page
 * 
 */
class Export_Page extends Export
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
    $this->page();
  }


  /**
   *
   * @return 
   * @access public
   */
  public function page( )
  {
    switch (@$_GET['d_u']) {
      case 'output':
        // @TODO
        break;

      case 'show':
      default:
        if (empty($_GET['d_r_id']) || strpos($_GET['d_r_id'], 'tr') >= 0) {
          // translation mode (contains "tr")
          $lang = $_GET['d_r_lang'];
        }
        else {
          $lang = $_GET['d_val2']
        }

        // remove "tr" so that it also work in translation view
        $data = str_replace('tr', '', $_GET['d_val']);

        if ( is_numeric($data) ) {
          $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id WHERE r.id = :rev_id  ORDER BY r.version DESC LIMIT 1");
          $stmt->bindParam('rev_id',$data,PDO::PARAM_INT);
        }
        else {
          $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id WHERE d.name = :data_name AND r.lang_id IN(:lang_one, :lang_two) ORDER BY r.version DESC LIMIT 1");
          $stmt->bindParam('data_name',$data,PDO::PARAM_STR);
          $stmt->bindParam('lang_one',$lang,PDO::PARAM_INT);
          $stmt->bindParam('lang_two',Language::getStandardId(),PDO::PARAM_INT);
        }

        $stmt->execute();
        $revision = $stmt->fetchOnce(PDO::FETCH_ASSOC);
        if (empty($_GET['d_val3'])) {
          $dynamic_num = Tag::getValueByUser($revision['id'], 'number', -1);
        }
        else {
          $dynamic_num = $_GET['d_val3'];
        }

        Log::writeGenerateLow('preview page: generate_page('.$revision['name'].', '.$revision['lang_id'].', '.$dynamic_num.', '.$_GET['d_u'].')');

        $export_html = new Export_HTML();
        echo $export_html->processText($revision['id'], $_GET['d_u']);
        break;
    }
  }


} // end of Export_Page
?>
