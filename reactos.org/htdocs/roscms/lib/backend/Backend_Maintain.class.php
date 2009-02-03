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
 * class Backend_Maintain
 * 
 * @package Branch_Maintain
 */
class Backend_Maintain extends Backend
{



  /**
   * choose action
   *
   * @access public
   */
  public function __construct( )
  {
    parent::__construct();

    switch ($_GET['action']) {
      case 'optimize':
        $this->optimize();
        break;

      case 'genall':
        $this->generateAll();
        break;

      case 'rebuilddepencies':
        $this->rebuildDepencies();
        break;

      case 'genone':
        $this->generateOne();
        break;
    } // end switch
  } // end of constructor



  /**
   * calls optimize on all database tables
   *
   * @access private
   */
  private function optimize( )
  {
    // call optimize table MySQL function
    $stmt=&DBConnection::getInstance()->prepare("OPTIMIZE TABLE ".ROSCMST_AREA.",".ROSCMST_USERS.",".ROSCMST_FORBIDDEN.",".ROSCMST_SESSIONS.",".ROSCMST_COUNTRIES.",".ROSCMST_ENTRIES.",".ROSCMST_ACCESS.",".ROSCMST_RIGHTS.",".ROSCMST_REVISIONS.",".ROSCMST_STEXT.",".ROSCMST_TAGS.",".ROSCMST_TEXT.",".ROSCMST_FILTER.",".ROSCMST_GROUPS.",".ROSCMST_JOBS.",".ROSCMST_LANGUAGES.",".ROSCMST_SUBSYS.",".ROSCMST_MEMBERSHIPS.",".ROSCMST_AREA_ACCESS.",".ROSCMST_DEPENCIES.",".ROSCMST_ACL.",".ROSCMST_TIMEZONES);
    $stmt->execute();

    Log::writeHigh('optimize database tables: done by '.ThisUser::getInstance()->id().' {data_maintain_out}');
  } // end of member function optimize



  /**
   * generate One selected entry for all languages
   *
   * @access private
   */
  private function generateOne( )
  {
    echo '<h4>Generate related pages of '.$_GET['name'].' ('.$_GET['data_type'].', '.$_GET['lang'].')</h4>';

    // try to get revision id
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id=d.id WHERE d.name = :name AND d.type = :type AND r.lang_id=:lang_id LIMIT 1");
    $stmt->bindParam('name', $_GET['name'],PDO::PARAM_STR);
    $stmt->bindParam('type', $_GET['data_type'],PDO::PARAM_STR);
    $stmt->bindParam('lang_id', $_GET['lang'],PDO::PARAM_INT);
    $stmt->execute();
    $rev_id = $stmt->fetchColumn();

    // we have an revision, now force generation
    if ($rev_id !== false) {
      $generate = new Generate();
      echo $generate->update($rev_id);
    }

    // report failure message
    else {
      echo 'Sorry, but we found no entry with the selected aspects';
    }
  } // end of member function generateOne



  /**
   * generate All pages
   *
   * @access private
   */
  private function generateAll( )
  {
    Log::writeGenerateHigh('generate all pages - start');

    // start timer
    $gentimeA = explode(' ',microtime()); 
    $gentimeA = $gentimeA[1] + $gentimeA[0]; 

    // generate pages
    $generate = new Generate();
    $generate->allEntries();

    // stop timer
    $gentimeB = explode(' ',microtime()); 
    $gentimeB = $gentimeB[1] + $gentimeB[0]; 

    // calculate run time
    $totaltimef = ($gentimeB - $gentimeA); 
    $showtimef = number_format($totaltimef, 4, '.', ''); 

    Log::writeGenerateHigh('generate all pages - end: '.$showtimef.' seconds');
  } // end of member function generateAll



  /**
   * generate All pages
   *
   * @access private
   */
  private function rebuildDepencies( )
  {
    Log::writeGenerateHigh('rebuilding depency tree - start');

    // start timer
    $gentimeA = explode(' ',microtime()); 
    $gentimeA = $gentimeA[1] + $gentimeA[0];

    // rebuild depencies
    $depencies = new Depencies();
    if ($depencies->rebuild()) {
      echo 'done';
    }
    else {
      echo 'Error: a problem occoured while rebuilding depencies';
    }

    // stop timer
    $gentimeB = explode(' ',microtime()); 
    $gentimeB = $gentimeB[1] + $gentimeB[0]; 

    // calculate run time
    $totaltimef = ($gentimeB - $gentimeA); 
    $showtimef = number_format($totaltimef, 4, '.', ''); 

    Log::writeGenerateHigh('rebuilding depency tree - end: '.$showtimef.' seconds');
  } // end of member function rebuildDepencies



} // end of Backend_Maintain
?>
