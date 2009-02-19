<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>
                  2009  Danny Götte <dangerground@web.de>

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
 * class Backend_ViewUserTable
 * 
 * @package Branch_Maintain
 * @subpackage Backend
 */
class Backend_ViewUserTable extends Backend
{

  private $sql_select = '';
  private $sql_from = '';
  private $sql_where = '';
  private $sql_order = '';

  private $page_limit = 25; // maximum number of entries per page
  private $column_list = '|';



  /**
   * set header, to do not cache
   *
   * @access public
   */
  public function __construct( )
  {
    // login and prevent caching for xml
    parent::__construct('xml');

    // filters
    $this->generateFilterSQL($_GET['d_filter'], $_GET['d_filter2']);

    // begin to construct xml
    $this->generateXML(@$_GET['d_cp']);
  } // end of constructor



  /**
   * outputs the XML code
   *
   * @param string filter
   * @access private
   */
  private function generateXML( $page_offset = 0 )
  {
    $thisuser = &ThisUser::getInstance();
    $row_counter = 1;

    // convert requested columns to array
    $this->column_list = substr($this->column_list,1,-1);// prevent from additional entries caused by '|' at start and end
    if ($this->column_list === false) {
      $column_array = array();
    }
    else {
      $column_array = explode('|', $this->column_list);
    }

    // check if there are entries which are found by filter settings
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ".ROSCMST_USERS." u ".$this->sql_from." WHERE 1 ".$this->sql_where);
   # echo "SELECT COUNT(*) FROM ".ROSCMST_USERS." u ".$this->sql_from." WHERE 1 ".$this->sql_where;
    $stmt->bindParam('archive',$this->archive_mode,PDO::PARAM_BOOL);
    $stmt->execute();
    $ptm_entries = $stmt->fetchColumn();

    echo '<?xml version="1.0" encoding="UTF-8"?><root>';

    // nothing was found
    if ($ptm_entries == false) {
      echo '#none#';
    }

    // start entry table
    else {
      // report entry count
      echo $ptm_entries.'<table>';

      // start table header
      echo '<view curpos="'.$page_offset.'" pagelimit="'.$this->page_limit.'" pagemax="'.$ptm_entries.'" tblcols="'.($this->column_list !== false ? '|'.$this->column_list.'|' : '').'" />';

      // make the order command ready for usage
      if (trim($this->sql_order) == '') {
        $this->sql_order = " ORDER BY u.created DESC";
      }
      else {
        $this->sql_order = " ORDER BY ".$this->sql_order;
      }

      // proceed entries
      $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT u.id, u.name, u.lang_id ".$this->sql_select." FROM ".ROSCMST_USERS." u ".$this->sql_from." WHERE 1 ".$this->sql_where." ".$this->sql_order." LIMIT :limit OFFSET :offset");
      $stmt->bindValue('limit',0+$this->page_limit,PDO::PARAM_INT);
      $stmt->bindValue('offset',0+$page_offset,PDO::PARAM_INT);
      $stmt->execute();
      while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $star_state = '0';
        $star_id = 0;
        $line_status = ($row_counter%2 ? 'odd' : 'even');
        $column_list_row = null;

        // construct column values for current row
        foreach ($column_array as $column) {
          $column_list_row .= '|';

          // handle column types
          switch ($column) {
            case 'E-mail':
              $column_list_row .= $row['email'];
              break;
            case 'Edits':
              $column_list_row .= $row['edited'];
              break;
            case 'Homepage':
              $column_list_row .= $row['homepage'];
              break;
            case 'Country':
              $column_list_row .= $row['country'];
              break;
            case 'Language':
              $column_list_row .= $row['language'];
              break;
            case 'Timezone':
              $column_list_row .= $row['timezone'];
              break;
            case 'Last-login':
              $column_list_row .= $row['modified'];
              break;
            case 'Registered':
              $column_list_row .= $row['created'];
              break;
            case 'Real-Name':
              $column_list_row .= $row['fullname'];
              break;
            default:
              $column_list_row .= 'Unknown ('.$column.')';
              break;
          }
        } // foreach
        if ($column_list_row !== null) {
          $column_list_row .= '|';
        }

        // table data
        echo '<row id="'.@$row['data_id2'].'"'
          .' dname="'.htmlspecialchars($row['name']).'"'
          .' type="'.@$row['type'].'"'
          .' rid="'.@$row['id'].'"'
          .' rver="'.@$row['version'].'"'
          .' rlang="'.$row['lang_id'].'"'
          .' rdate="'.@$row['created'].'"'
          .' rusrid="'.$row['id'].'"'
          .' star="'. $star_state .'"' /* starred (on=1, off=0) */
          .' starid="'. @$star_id .'"' /* star tag id (from getTagId(...)) */
          .' status="'. $line_status .'"' /* status (odd/even (=stable), new, draft, etc.) */
          .' security="write"' /* security (read, write, add, pub, trans) */
          .' xtrcol="'.$column_list_row.'"'
          .'><![CDATA['.urlencode(@$row['fullname']).']]></row>';

        ++$row_counter;
      } // while
      echo '  </table>';
    }
    echo '</root>';
  } // end of member function generateXML



  /**
   * build filter settings as SQL stored in class vars
   *
   * @param string filter
   * @access private
   */
  private function generateFilterSQL( $searchtext, $filter )
  {
    $thisuser = &ThisUser::getInstance();
  
    // check if there is something to do
    if ($filter == '') {
      return;
    }

    // helper vars
    $type_single_counter = array();
    $type_multiple_counter = array();
    $memberships = '';
    $searchby = false;
    $edits = false;

    // seperate the filters
    $filter_list = explode('|',$filter); 
    asort($filter_list);


    // get count of single filter types
    foreach ($filter_list as $key => $value) { // AND/OR algo
      $filter_type = substr($value, 0, 1);
      if (array_key_exists($filter_type, $type_single_counter)) {
        $type_single_counter[$filter_type] = $type_single_counter[$filter_type] + 1;
      }
      else {
        $type_single_counter[$filter_type] = 1;
      }
    }

    // handle filters
    foreach ($filter_list as $key => $value) {
      $filter_part = explode('_',strtolower($value)); 
      $type_a = $filter_part[0];
      $type_b = @$filter_part[1];
      $type_c = @$filter_part[2];

      // handle filter types (single filters)
      if ($type_a == 'c' || $type_a == 'o' || $type_a == 'b') {
        switch ($type_a) {

          // search by
          case 'b':
            // find the search by modifier, evaluate it at the end of this method
            $searchby = $type_c;
            break;

          // add to columns list
          case 'c':
            switch ($type_c) {
              case 'e-mail':
                $this->sql_select .= ", u.email";
                break;
              case 'homepage':
                $this->sql_select .= ", u.homepage";
                break;
              case 'last-login':
                $this->sql_select .= ", u.modified";
                break;
              case 'registered':
                $this->sql_select .= ", u.created";
                break;
              case 'neal-name':
                $this->sql_select .= ", u.fullname";
                break;
              case 'edits':
                $edits = true; // be sure to include edits
                break;
              case 'country':
                $this->sql_from .= " JOIN ".ROSCMST_COUNTRIES." c ON c.id=u.country_id";
                $this->sql_select .= ", c.name AS country";
                break;
              case 'language':
                $this->sql_from .= " JOIN ".ROSCMST_LANGUAGES." l ON l.id=u.lang_id";
                $this->sql_select .= ", l.name AS language";
                break;
              case 'timezone':
                $this->sql_from .= " JOIN ".ROSCMST_TIMEZONES." t ON t.id=u.timezone_id";
                $this->sql_select .= ", CONCAT(t.name_short,'(',t.difference,')') AS timezone";
                break;
            } // end switch type_c
            $this->column_list .= ucfirst($type_c) .'|';
            break;

          // order
          case 'o':
            if ($this->sql_order != '') {
              $this->sql_order .= ", ";
            }

            // set order field
            switch ($type_c) {
              case 'name': // accóunt name
                $this->sql_order .= " u.name ";
                break;
              case 'real-name': // real name
                $this->sql_order .= " u.fullname ";
                break;
              case 'e-mail': // E-Mail
                $this->sql_order .= "  u.email ";
                break;
              case 'homepage': // Homepage
                $this->sql_order .= " u.homepage ";
                break;
              case 'country': // Country
                $this->sql_from .= " JOIN ".ROSCMST_COUNTRY." c ON c.id=u.country_id ";
                $this->sql_order .= "c.name ";
                break;
              case 'language': // Language
                $this->sql_from .= " JOIN ".ROSCMST_COUNTRY." l ON l.id=u.lang_id ";
                $this->sql_order .= "l.name ";
                break;
              case 'registered': // Registered
                $this->sql_order .= "u.created ";
                break;
              case 'edits': // Edits
                $edits = true; // be sure to include edits
                $this->sql_order .= "edited ";
                break;
              case 'last-login': // Last login
                $this->sql_order .= "u.modified ";
                break;
              default:
                echo $filter_part;
                break;
            }

            // set standard order by direction
            if ($type_b == 'desc') {
              $this->sql_order .= ' DESC ';
            }
            else {
              $this->sql_order .= ' ASC ';
            }
            break;

          // kind
          case 'k': 
            $this->archive_mode = true;
            break;
        } // end switch
      }
      // multiple filters 'AND (foo OR bar)'
      else {

        // group filter settings
        if ($type_single_counter[$type_a] > 1 && !array_key_exists($type_a, $type_multiple_counter)) {
          $this->sql_where .= " AND ( ";
          $type_multiple_counter[$type_a] = 1;
        }
        elseif (array_key_exists($type_a, $type_multiple_counter) && $type_multiple_counter[$type_a] < $type_single_counter[$type_a]) {
          $this->sql_where .= " OR ";
          ++$type_multiple_counter[$type_a];
        }
        else {
          $this->sql_where .= " AND ";
          if (array_key_exists($type_a, $type_multiple_counter)) {
            ++$type_multiple_counter[$type_a];
          }
        }

        // handle filter types (multiple filters)
        switch($type_a) {

          // language
          case 'l': 
            $this->sql_where .= "u.lang_id".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
            break;

          // groups
          case 'g':
            $memberships = true; // load membership table
            $this->sql_where .= " m.group_id = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
            break;
        } // switch

        // close filter group
        if (array_key_exists($type_a, $type_multiple_counter) && $type_single_counter[$type_a] == $type_multiple_counter[$type_a]) {
          $this->sql_where .= " ) ";
        }
      }
    } // foreach

    // we need edits
    if ($edits) {
      $this->sql_select .= ", (SELECT COUNT(id) FROM ".ROSCMST_REVISIONS." WHERE user_id = u.id AND version > 0) AS edited";
    }

    // ask for memberships
    if ($memberships !== '') {
      $this->sql_from .= " JOIN ".ROSCMST_MEMBERSHIPS." m ON m.user_id=u.id ";
    }

    // search by ??? or not (if no term is given)
    if (empty($_GET['d_filter'])) {
      return;
    }

    $this->sql_where .= " AND ";

    switch ($searchby) {
      case 'rname': // real name
        $this->sql_where .= " u.fullname";
        break;
      case 'email': // account name
        $this->sql_where .= " u.email";
        break;
      case 'web': // homepage
        $this->sql_where .= " u.homepage";
        break;
      case 'name': // account name
      default:
        $this->sql_where .= " u.name";
        break;
    } // end switch
    $this->sql_where .= " LIKE ".DBConnection::getInstance()->quote('%'.$searchtext.'%',PDO::PARAM_STR)." ";

  } // end of member function generateFilterSQL



} // end of Backend_ViewUserTable
?>
