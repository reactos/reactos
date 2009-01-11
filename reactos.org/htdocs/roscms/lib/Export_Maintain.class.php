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
    switch ($_GET['d_u']) {
      case 'optimize':
        $stmt=&DBConnection::getInstance()->prepare("OPTIMIZE TABLE ".ROSCMST_ACCESS.",".ROSCMST_USERS.",".ROSCMST_FORBIDDEN.",".ROSCMST_SESSIONS.",".ROSCMST_COUNTRIES.",".ROSCMST_ENTRIES.",".ROSCMST_REVISIONS.",".ROSCMST_STEXT.",".ROSCMST_TAGS.",".ROSCMST_TEXT.",".ROSCMST_FILTER.",".ROSCMST_GROUPS.",".ROSCMST_JOBS.",".ROSCMST_LANGUAGES.",".ROSCMST_SUBSYS.",".ROSCMST_MEMBERSHIPS.",".ROSCMST_ENTRY_AREA.",".ROSCMST_DEPENCIES.",".ROSCMST_TIMEZONES);
        $stmt->execute();
        Log::writeHigh('optimize database tables: done by '.ThisUser::getInstance()->id().' {data_maintain_out}');
        break;

      case 'genpages':
        $gentimeA = explode(' ',microtime()); 
        $gentimeA = $gentimeA[1] + $gentimeA[0]; 

        Log::writeGenerateHigh('generate all pages - start');

        $generate = new Generate();
        $generate->allEntries();

        $gentimeB = explode(' ',microtime()); 
        $gentimeB = $gentimeB[1] + $gentimeB[0]; 

        $totaltimef = ($gentimeB - $gentimeA); 
        $showtimef = number_format($totaltimef, 4, '.', ''); 

        Log::writeGenerateHigh('generate all pages - end: '.$showtimef.' seconds');
        break;

      case 'rebuilddepencies':
        $gentimeA = explode(' ',microtime()); 
        $gentimeA = $gentimeA[1] + $gentimeA[0];

        Log::writeGenerateHigh('rebuilding depency tree - start');

        $data = new DataDepencies();
        $data->rebuildDepencies();

        $gentimeB = explode(' ',microtime()); 
        $gentimeB = $gentimeB[1] + $gentimeB[0]; 

        $totaltimef = ($gentimeB - $gentimeA); 
        $showtimef = number_format($totaltimef, 4, '.', ''); 

        Log::writeGenerateHigh('rebuilding depency tree - end: '.$showtimef.' seconds');
        break;

      case 'pubdate':
        echo '<h4>Generate related pages of '.$_GET['d_val'].' ('.$_GET['d_val2'].', '.$_GET['d_val3'].', '.$_GET['d_val4'].')</h4>';

        $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ENTRIES." WHERE name = :name AND type = :type LIMIT 1");
        $stmt->bindParam('name', $_GET['d_val'],PDO::PARAM_STR);
        $stmt->bindParam('type', $_GET['d_val2'],PDO::PARAM_STR);
        $stmt->execute();
        $data_id = $stmt->fetchColumn();

        $generate = new Generate();
        echo $generate->generateAll($data_id, 'data');
        break;
    } // end switch
  }


} // end of Export_Maintain
?>
