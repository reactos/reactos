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
 * class Export_XML
 * 
 */
class Export_XML extends Export
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/

  private $sql_select = '';

  private $sql_from = '';

  private $sql_where = '';

  private $sql_order = '';

  private $translation_lang = ''; // holds language name if in translation mode

  private $page_limit = 25; // maximum number of entries per page

  private $column_list = '|';

  private $archive_mode = false; // boolean



  /**
   * process requests from external classes
   *
   * @param string area 'ptm'
'ptm' = page table main (list of entries)
   * @return 
   * @access public
   */
  public function __construct( $area )
  {
    parent::__construct();

    switch ($area) {
      case 'ptm': // page table main
        self::page_table_main(htmlspecialchars(@$_GET["d_filter"]), htmlspecialchars(@$_GET["d_filter2"]), @$_GET["d_cp"]);
        break;
      default:
        die('');
        break;
    }
  } // end of member function __construct


  /**
   * Constructs the list of entries with help of filters
   *
   * @param int curpos

   * @param string data_name searches for data name

   * @param string filter
   * @return 
   * @access public
   */
  public function page_table_main( $data_name, $filter, $page_offset = 0 )
  {
    // set headers, do not cache !
    header('Content-type: text/xml');
    header('Expires: Mon, 26 Jul 1997 05:00:00 GMT');    // Date in the past
    header('Last-Modified: '.gmdate('D, d M Y H:i:s').' GMT'); // always modified
    header('Cache-Control: no-store, no-cache, must-revalidate');  // HTTP/1.1
    header('Cache-Control: post-check=0, pre-check=0', false);
    header('Pragma: no-cache');                          // HTTP/1.0col

    // search for data name
    if ($data_name != '') {
      $this->sql_where = " AND d.name LIKE ".DBConnection::getInstance()->quote('%'.$data_name.'%',PDO::PARAM_STR)." ";
    }

    // filters
    self::generateFilterSQL($filter);

    // begin to construct xml
    self::generateXML($page_offset);

  } // end of member function page_table_main


  /**
   * outputs the XML code
   *
   * @param string filter

   * @return 
   * @access private
   */
  private function generateXML( $page_offset = 0 )
  {
    $thisuser = &ThisUser::getInstance();

    $tdata = '';
    $row_counter = 1;
    $this->column_list = substr($this->column_list,1,-1);// prevent from additional entries caused by '|' at start and end
    if ($this->column_list === '') {
      $column_array = array();
    }
    else {
      $column_array = explode('|', $this->column_list);
    }

    // check if there are entries which are found by filter settings
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON r.data_id = d.id ".$this->sql_from." WHERE r.version >= 0 AND r.archive = :archive AND d.access_id IN(".Data::hasAccessAsList('read').") ".$this->sql_where);
    $stmt->bindParam('archive',$this->archive_mode,PDO::PARAM_BOOL);
    $stmt->execute();
    $ptm_entries = $stmt->fetchColumn();

    echo '<?xml version="1.0" encoding="UTF-8"?><root>';
    if ($ptm_entries == false) {
      echo '#none#';
    }
    else {
      // start information
      echo $ptm_entries.'<table>';

      // start table header
      $tdata .= '<view curpos="'.$page_offset.'" pagelimit="'.$this->page_limit.'" pagemax="'.$ptm_entries.'" tblcols="|'.$this->column_list.'|" />';

      // prepare for usage in loop
      $stmt_trans=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, d.type, r.id, r.version, r.lang_id, r.datetime, r.user_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id=d.id WHERE d.id = :data_id AND r.version > 0 AND r.lang_id = :lang AND r.archive = :archive LIMIT 1");
      $stmt_trans->bindParam('archive',$this->archive_mode,PDO::PARAM_BOOL);
      $stmt_trans->bindParam('lang',$this->translation_lang,PDO::PARAM_INT);
      $stmt_stext=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_STEXT." WHERE rev_id = :rev_id AND name = 'title' LIMIT 1");
      $stmt_lang=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_LANGUAGES." WHERE id = :lang LIMIT 1");
      $stmt_user=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_USERS." WHERE id = :user_id LIMIT 1");
      $stmt_acl=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_ACCESS." WHERE id = :access_id LIMIT 1");

      // make the order command ready for usage
      if ($this->sql_order == null) {
        $this->sql_order = " ORDER BY r.id DESC";
      }
      else {
        $this->sql_order = " ORDER BY ".$this->sql_order;
      }

      // proceed entries
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.data_id, d.name, d.type, d.access_id, r.id, r.version, r.lang_id, r.datetime, r.user_id ".$this->sql_select." FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON r.data_id = d.id ".$this->sql_from." WHERE r.version >= 0 AND r.archive = :archive AND d.access_id IN(".Data::hasAccessAsList('read').") ".$this->sql_where." ".$this->sql_order." LIMIT :limit OFFSET :offset");
      $stmt->bindParam('archive',$this->archive_mode,PDO::PARAM_BOOL);
      $stmt->bindValue('limit',0+$this->page_limit,PDO::PARAM_INT);
      $stmt->bindValue('offset',0+$page_offset,PDO::PARAM_INT);
      $stmt->execute();
      while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $star_state = '0';
        $star_id = 0;
        $line_status = 'unknown';
        $column_list_row = '';
        $security = '';

        // for non translation
        if ($this->translation_lang == '') {
          $entry_status = Tag::getValueByUser($row['id'], 'status', -1);

          // get line status/color
          if ($entry_status == 'stable') {
            $line_status = ($row_counter%2 ? 'odd' : 'even');
          }
          elseif ($entry_status == 'new') {
            $line_status = 'new';
          }
          elseif ($entry_status == 'draft') {
            $line_status = 'draft';
          }
          $row['data_id2'] = $row['data_id'];
          
        }
        // translation
        else {

          // check if translated article exists
          $stmt_trans->bindParam('data_id',$row['data_id'],PDO::PARAM_INT);
          $stmt_trans->execute();
          $translated_entry = $stmt_trans->fetchOnce(PDO::FETCH_ASSOC);

          // translation doesn't exist, so enable "translate mode"
          if ($translated_entry['datetime'] == "") {
            $line_status = 'transb';

            // figure out if user can translate things
            if (Data::hasAccess($row['data_id'], 'translate')) {
              $row['data_id2'] = 'tr'.$row['data_id'];
              $row['id'] = 'tr'.$row['id'];
              $row['datetime'] = 'translate!';
            }
            else {
              $row['data_id2'] = 'notrans';
              $row['id'] = 'notrans';
              $row['datetime'] = '-';
            }

            // set info for non existent translations
            $row['version'] = 0;
            $row['user_id'] = '';
          }
          // translation already exists
          else {
            if ($row['datetime'] > $translated_entry['datetime']) {
              $line_status = 'transg';
            }
            else {
              $line_status = 'transr';
            }
            $row = $translated_entry;
            $row['data_id2'] = $translated_entry['data_id'];
          }
        }

        // care about bookmark visibility
        if (Tag::getValueByUser($row['id'], 'star', $thisuser->id()) == 'on') {
          $star_state = '1';
        }
        $star_id = Tag::getIdByUser($row['id'], 'star', $thisuser->id());

        // get page title
        $stmt_stext->bindParam('rev_id',$row['id'],PDO::PARAM_INT);
        $stmt_stext->execute();
        $stext_content = $stmt_stext->fetchColumn();

        // construct column values for current row
        foreach ($column_array as $column) {
          $column_list_row .= '|';

          // handle column types
          switch ($column) {
            case 'Language':
              $stmt_lang->bindParam('lang',$row['lang_id'],PDO::PARAM_STR);
              $stmt_lang->execute();
              $language = $stmt_lang->fetchColumn();
              if ($language != '') {
                $column_list_row .= $language;
              }
              else {
                $column_list_row .= '? (Unknown)';
              }
              break;
            case 'User':
              $stmt_user->bindParam('user_id',$row['user_id'],PDO::PARAM_INT);
              $stmt_user->execute();
              $user_name = $stmt_user->fetchColumn();
              if ($user_name != '') {
                $column_list_row .= $user_name;
              }
              else {
                $column_list_row .= 'Unknown';
              }
              break;
            case 'Type':
              $column_list_row .= $row['type'];
              break;
            case 'Security':
              $stmt_acl->bindParam('access_id',$row['access_id'],PDO::PARAM_INT);
              $stmt_acl->execute();
              $acl = $stmt_acl->fetchColumn();
              if ($acl != '') {
                $column_list_row .= $acl;
              }
              else {
                $column_list_row .= 'Unknown';
              }
              break;
            case 'Rights':
              $column_list_row .= 'Depracted';
              break;
            case 'Version':
              $column_list_row .= $row['version'] ;
              break;
            default:
              $column_list_row .= 'Unknown';
              break;
          }
        } // foreach
        $column_list_row .= '|';

        // has person right to write / edit entries ?
        if (Data::hasAccess($row['data_id'], 'edit')) {
          $security = 'write';
        }

        // table data
        $tdata .= '<row id="'.$row['data_id2'].'"';
        $tdata .= ' dname="'.$row['name'].'"';
        $tdata .= ' type="'.$row['type'].'"';
        $tdata .= ' rid="'.$row['id'].'"';
        $tdata .= ' rver="'.$row['version'].'"';
        $tdata .= ' rlang="'.$row['lang_id'].'"';
        $tdata .= ' rdate="'.$row['datetime'].'"';
        $tdata .= ' rusrid="'.$row['user_id'].'"';
        $tdata .= ' star="'. $star_state .'"'; /* starred (on=1, off=0) */
        $tdata .= ' starid="'. $star_id .'"'; /* star tag id (from getTagId(...)) */
        $tdata .= ' status="'. $line_status .'"'; /* status (odd/even (=stable), new, draft, etc.) */
        $tdata .= ' security="'. $security .'"'; /* security (read, write, add, pub, trans) */
        $tdata .= ' xtrcol="'.$column_list_row.'"';
        $tdata .= '><![CDATA['.urlencode(substr($stext_content, 0, 30)).']]></row>';

        $row_counter++;
      } // while
      echo $tdata;
      echo '  </table>';
    }
    echo '</root>';
  } // end of member function generateXML


  /**
   * 
   *
   * @param string filter
   * @return 
   * @access private
   */
  private function generateFilterSQL( $filter )
  {
    $thisuser = &ThisUser::getInstance();
  
    // check if there is something to do
    if ($filter == '') {
      return;
    }

    // helper vars
    $type_single_counter = array();
    $type_multiple_counter = array();
    $tag_counter = 0;
    $entries_private = 0;
    $entries_public = 0;
    $entries_system = 0;

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
      $filter_part = explode('_',$value); 
      $type_a = $filter_part[0];
      $type_b = @$filter_part[1];
      $type_c = @$filter_part[2];

      // handle filter types (single filters)
      if ($type_a == 'r' || $type_a == 'c' || $type_a == 'o' || ($type_a == 'k' && $type_c == 'archive')) {
        switch ($type_a) {

          // set translation language
          case 'r': 
            if ($this->translation_lang == '' & $type_c != '') { // only one such filter is allowed
              $this->translation_lang = $type_c;
            }
            break;

          // add to columns list
          case 'c': 
            $this->column_list .= ucfirst($type_c) .'|';
            break;

          // order
          case 'o':
            if ($this->sql_order != '') {
              $this->sql_order .= ", ";
            }

            // set order field
            switch ($type_c) {
              case 'datetime': // date-time
                $this->sql_order .= " r.datetime ";
                break;
              case 'name': // name
                $this->sql_order .= "d.name ";
                break;
              case 'lang': // language
                $this->sql_order .= "r.lang_id ";
                break;
              case 'usr': // user
                $this->sql_order .= "r.user_id ";
                break;
              case 'ver': // version
                $this->sql_order .= "r.version ";
                break;
              case 'nbr': // number ("dynamic" entry)
                $tag_counter++;
                $this->sql_order .= " CAST(t".$tag_counter.".value AS INT)";
                $this->sql_where .= " AND t".$tag_counter.".name = 'number' ";
                break;
              case 'type': // type
                $this->sql_order .= "d.type ";
                break;
              case 'security': // security (ACL)
                $this->sql_order .= "d.access_id ";
                break;
              case 'revid': // revision-id
                $this->sql_order .= "r.id ";
                break;
              case 'ext': // page extension 
                $tag_counter++;
                $this->sql_order .= " t".$tag_counter.".value ";
                $this->sql_where .= " AND t".$tag_counter.".name = 'extension' ";
                break;
              case 'status': // status 
                $tag_counter++;
                $this->sql_order .= " t".$tag_counter.".value ";
                $this->sql_where .= " AND t".$tag_counter.".name = 'status' ";
                break;
              case 'kind': // kind 
                $tag_counter++;
                $this->sql_order .= " t".$tag_counter.".value ";
                $this->sql_where .= " AND t".$tag_counter.".name = 'kind' ";
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
          $type_multiple_counter[$type_a]++;
        }
        else {
          $this->sql_where .= " AND ";
          if (array_key_exists($type_a, $type_multiple_counter)) {
            $type_multiple_counter[$type_a]++;
          }
        }

        // handle filter types (multiple filters)
        switch($type_a) {

          // kind (stable, new, draft, unknown)
          case 'k': 
            if ($type_c == 'draft') {
              $entries_private++;
            }
            elseif ($type_c == 'system') {
              $entries_system++;
            }
            else {
              $entries_public++;
            }

              // ask for status-tag value
              $tag_counter++;
              $this->sql_where .= " (t".$tag_counter.".name = 'status' AND t".$tag_counter.".value".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR).") ";
            break;

          // tag
          case 'a': 
            $tag_counter++;
            $this->sql_where .= " (t".$tag_counter.".name = 'tag' AND t".$tag_counter.".value".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR).") ";
            break;

          // entry name
          case 'n': 
            $this->sql_where .= "d.name";

            // matching method
            switch ($type_b) {
              case 'is':
                $this->sql_where .= " = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR)." ";
                break;
              case 'likea':
                $this->sql_where .= " LIKE ".DBConnection::getInstance()->quote('%'.$type_c.'%',PDO::PARAM_STR)." ";
                break;
              case 'likeb':
                $this->sql_where .= " LIKE ".DBConnection::getInstance()->quote($type_c.'%',PDO::PARAM_STR)." ";
                break;
              default:
                $this->sql_where .= " != ".DBConnection::getInstance()->quote($roscms_d_f2_typec,PDO::PARAM_STR)." ";
                break;
            }
            break;

          // type   (page, content, template, script, system)
          case 'y':
            $this->sql_where .= "d.type".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
            break;

          // starred
          case 's': 
            $tag_counter++;
            $this->sql_where .= " (t".$tag_counter.".name = 'star' AND t".$tag_counter.".value = '".($type_c=='true'?'on':'off')."') ";
            break;

          // date
          case 'd': 
            $this->sql_where .= "DATE(r.datetime)";

            // matching method
            switch ($type_b) {
              case 'is':
                $this->sql_where .= " = ";
                break;
              case 'sm':
                $this->sql_where .= " < ";
                break;
              case 'la':
                $this->sql_where .= " > ";
                break;
              default:
                $this->sql_where .= " != ";
                break;
            }
            $this->sql_where .= DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
            break;

          // time
          case 't': 
            $this->sql_where .= "TIME(r.datetime)";

            // matching method
            switch ($type_b) {
              case 'is':
                $this->sql_where .= " = ";
                break;
              case 'sm':
                $this->sql_where .= " < ";
                break;
              case 'la':
                $this->sql_where .= " > ";
                break;
              default:
                $this->sql_where .= " != ";
                break;
            }
            $this->sql_where .= DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
            break;

          // language
          case 'l': 
            $this->sql_where .= "r.lang_id".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
            break;

          // security (ACL)
          case 'i': 
            $this->sql_where .= "d.access_id ".($type_b=='is' ?'':"NOT ")."IN (".Data::hasAccessAsList($type_c).")";
            break;

          // metadata
          case 'm': 
            $tag_counter++;
            $this->sql_where .= " (t".$tag_counter.".name = ".DBConnection::getInstance()->quote(strtolower($type_b),PDO::PARAM_STR)." AND t".$tag_counter.".value = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR).") ";
            break;

          // user
          case 'u': 
            // get user_id
            $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_USERS." WHERE name = :user_name LIMIT 1");
            $stmt->bindParam('user_name',$type_c,PDO::PARAM_STR);
            $stmt->execute();
            $user_id = $stmt->fetchColumn();

            $this->sql_where .= "r.user_id".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($user_id,PDO::PARAM_INT);
            break;

          // version
          case 'v': 
            $this->sql_where .= "r.version";

            // matching method
            switch ($type_b) {
              case 'is':
                $this->sql_where .= " = ";
                break;
              case 'sm':
                $this->sql_where .= " < ";
                break;
              case 'la':
                $this->sql_where .= " > ";
                break;
              default:
                $this->sql_where .= " != ";
                break;
            }
            $this->sql_where .= DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
            break;

          // system
          case 'e': 
            switch ($type_b) {
              case 'revid':
                $this->sql_where .= "r.id = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
                break;
              case 'usrid':
                $this->sql_where .= "r.user_id = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
                break;
              case 'langid':
                $this->sql_where .= "r.lang_id = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
                break;
              case 'dataid':
              default:
                $this->sql_where .= "r.data_id = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
                break;
            }
            break;
        } // switch

        // close filter group
        if (array_key_exists($type_a, $type_multiple_counter) && $type_single_counter[$type_a] == $type_multiple_counter[$type_a]) {
          $this->sql_where .= " ) ";
        }
      }
    } // foreach

    // if no filter is set, construct a new one
    if ($entries_private <= 0 && $entries_system <= 0 && $entries_public <= 0) { 

      // new, stable
      $this->sql_where .= " AND (t.name = 'status' AND (t.value = 'new' OR t.value = 'stable')) ";

      // set additional needed sql
      $this->sql_select .= ", t.name AS tag_name, t.value AS tag_value ";
      $this->sql_from .= " JOIN ".ROSCMST_TAGS." t ON t.rev_id=r.id ";
      $this->sql_where .= " AND t.user_id IN(-1, 0, ".DBConnection::getInstance()->quote($thisuser->id(),PDO::PARAM_INT).") ";
    }

    // construct additioanl sql for tag-usage from filter
    if ($tag_counter > 0) {
      $this->sql_select .= ", ".$tag_counter." AS tag_count";
      for ($i = 1; $i <= $tag_counter; $i++) {
        $this->sql_select .= ", t".$i.".name AS tag_name".$i.", t".$i.".value AS tag_value".$i." ";
        $this->sql_from .= " JOIN ".ROSCMST_TAGS." t".$i." ON t".$i.".rev_id = r.id ";
        $this->sql_where .= " AND t".$i.".user_id IN(-1, 0, ".DBConnection::getInstance()->quote($thisuser->id(),PDO::PARAM_INT).") ";
      }
    }

    // make sure only private drafts are visible
    if (!$thisuser->hasAccess('other_drafts') && $entries_private > 0) {
      $this->sql_where .= " AND r.user_id = '".$thisuser->id()."' ";
    }

    // either show draft (private) OR stable & new (public) entries,   private AND public entries together are NOT allowed => block 
    if (($entries_private > 0 && $entries_public > 0 && $thisuser->hasAccess('mix_priv_pub')) || ($thisuser->hasAccess('show_sys_entry') && $entries_system > 0)) {
      $this->sql_select = "";
      $this->sql_from   = "";
      $this->sql_where  = " FALSE ";
      $this->sql_order  = "";
    }

  } // end of member function generateFilterSQL

} // end of Export_XML
?>
