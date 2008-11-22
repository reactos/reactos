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

  private $a = ''; // holds '_a' if in archive mode

  private $a2 = ''; // holds 'a' if in archive mode




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
      $this->sql_where = " AND d.data_name LIKE ".DBConnection::getInstance()->quote('%'.$data_name.'%',PDO::PARAM_STR)." ";
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
    $stmt=DBConnection::getInstance()->prepare("SELECT COUNT('d.data_id') FROM data_revision".$this->a." r, data_".$this->a2." d ".$this->sql_from." , data_security y WHERE r.rev_version >= 0 AND r.data_id = d.data_id  AND d.data_acl = y.sec_name AND y.sec_branch = 'website' ". Security::getACL('read') ." ". $this->sql_where);
    $stmt->execute();
    $ptm_entries = $stmt->fetchColumn();

    echo '<?xml version="1.0" encoding="UTF-8"?><root>';
    if (!$ptm_entries) {
      echo '#none#';
    }
    else {
      // start information
      echo $ptm_entries.'<table>';

      // start table header
      $tdata .= "    <view curpos=\"".$page_offset."\" pagelimit=\"".$this->page_limit."\" pagemax=\"".$ptm_entries."\" tblcols=\"|".$this->column_list."|\" /> \n";

      // prepare for usage in loop
      $stmt_trans=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, r.rev_date, r.rev_usrid FROM data_".$this->a2." d, data_revision".$this->a." r WHERE d.data_id = :data_id AND r.rev_version > 0 AND d.data_id = r.data_id AND r.rev_language = :lang LIMIT 1");
      $stmt_trans->bindParam('lang',$this->translation_lang);
      $stmt_trans_dyn=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, d.data_type, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, r.rev_date, r.rev_usrid FROM data_".$this->a2." d JOIN data_revision".$this->a." r ON d.data_id = r.data_id JOIN data_tag".$this->a." a ON (r.data_id = a.data_id AND r.rev_id = a.data_rev_id) JOIN data_tag_name".$this->a." n ON a.tag_name_id = n.tn_id JOIN data_tag_value".$this->a." v ON a.tag_value_id  = v.tv_id WHERE d.data_id = :data_id AND r.rev_version > 0 AND  r.rev_language = :lang AND a.tag_usrid = '-1' AND v.tv_value = :dyn_num LIMIT 1");
      $stmt_trans_dyn->bindParam('lang',$this->translation_lang);
      $stmt_stext=DBConnection::getInstance()->prepare("SELECT stext_content FROM data_stext".$this->a." WHERE data_rev_id = :rev_id AND stext_name = 'title' LIMIT 1");
      $stmt_lang=DBConnection::getInstance()->prepare("SELECT lang_name FROM languages WHERE lang_id = :lang LIMIT 1");
      $stmt_user=DBConnection::getInstance()->prepare("SELECT user_name FROM users WHERE user_id = :user_id LIMIT 1");

      // make the order command ready for usage
      if ($this->sql_order == '') {
        echo 'guakdf';
        $this->sql_order = " ORDER BY r.rev_id DESC";
      }
      else {
        $this->sql_order = " ORDER BY ".$this->sql_order;
      }

      // proceed entries
      $stmt=DBConnection::getInstance()->prepare("SELECT d.data_id, d.data_name, d.data_type, d.data_acl, r.rev_id, r.rev_version, r.rev_language, r.rev_datetime, r.rev_date, r.rev_usrid  ".$this->sql_select." , y.sec_lev".$thisuser->securityLevel()."_write FROM data_revision".$this->a." r, data_".$this->a2." d ".$this->sql_from." , data_security y WHERE r.rev_version >= 0 AND r.data_id = d.data_id AND d.data_acl = y.sec_name AND y.sec_branch = 'website' ". Security::getACL('read') ." ". $this->sql_where ." ". $this->sql_order ." LIMIT :limit OFFSET :offset");
      $stmt->bindValue('limit',0+$this->page_limit,PDO::PARAM_INT);
      $stmt->bindValue('offset',0+$page_offset,PDO::PARAM_INT);
      $stmt->execute();
      while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $star_state = '0';
        $star_id = 0;
        $line_status = 'unknown';
        $column_list_row = '';
        $security = '';

        // try to get a dynamic number, from newsletters, news, ...
        $dynamic_num = Tag::getValueByUser($row['data_id'], $row['rev_id'], 'number', -1);

        // for non translation
        if ($this->translation_lang == '') {
          $entry_status = Tag::getValueByUser($row['data_id'], $row['rev_id'], 'status', -1);

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
          if ($dynamic_num > 0) {
            $stmt_translation=$stmt_trans_dyn;
            $stmt_translation->bindParam('dyn_num',$dynamic_num,PDO::PARAM_STR);
          } else {
            $stmt_translation=$stmt_trans;
          }
          $stmt_translation->bindParam('data_id',$row['data_id']);
          $stmt_translation->execute();
          $translated_entry = $stmt_translation->fetchOnce(PDO::FETCH_ASSOC);

          // translation doesn't exist, so enable "translate mode"
          if ($translated_entry['rev_datetime'] == "") { 
            $line_status = "transb";

            // figure out if user can translate things
            if (Security::hasRight($row['data_id'], 'trans')) {
              $row['data_id2'] = 'tr'.$row['data_id'];
              $row['rev_id'] = 'tr'.$row['rev_id'];
              $row['rev_datetime'] = 'translate!';
            }
            else {
              $row['data_id2'] = 'notrans';
              $row['rev_id'] = 'notrans';
              $row['rev_datetime'] = '-';
            }

            // set info for non existent translations
            $row['rev_version'] = 1;
            $row['rev_usrid'] = '';
          }
          // translation already exists
          else {
            if ($row['rev_date'] > $translated_entry['rev_date']) {
              $line_status = 'transg';
            }
            else {
              $line_status = 'transr';
            }
            $row = $translated_entry;
            $row['data_id2'] = $translated_entry['data_id'];
          }
        }

        // prepare dynamic entries
        if ($row['data_type'] == 'content' && $dynamic_num > 0) {
          $row['data_name'] .= '_'.$dynamic_num;
        }

        // care about bookmark visibility
        if (Tag::getValueByUser($row['data_id'], $row['rev_id'], 'star', $thisuser->id()) == 'on') {
          $star_state = '1';
        }
        $star_id = Tag::getIdByUser($row['data_id'], $row['rev_id'], 'star', $thisuser->id());

        // get page title
        $stmt_stext->bindParam('rev_id',$row['rev_id'],PDO::PARAM_INT);
        $stmt_stext->execute();
        $stext_content = $stmt_stext->fetchColumn();

        // construct column values for current row
        foreach ($column_array as $column) {
          $column_list_row .= '|';

          // handle column types
          switch ($column) {
            case 'Language':
              $stmt_lang->bindParam('lang',$row['rev_language'],PDO::PARAM_STR);
              $stmt_lang->execute();
              $language = $stmt_lang->fetchColumn();
              if ($language != '') {
                $column_list_row .= $language;
              }
              else {
                $column_list_row .= '? ('.$row['rev_language'].')';
              }
              break;
            case 'User':
              $stmt_user->bindParam('user_id',$row['rev_usrid'],PDO::PARAM_INT);
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
              $column_list_row .= $row['data_type'];
              break;
            case 'Security':
              $column_list_row .= $row['data_acl'];
              break;
            case 'Rights':
              $column_list_row .= Security::rightsOverview($row['data_id']);
              break;
            case 'Version':
              $column_list_row .= $row['rev_version'] ;
              break;
            default:
              $column_list_row .= 'Unknown';
              break;
          }
        } // foreach
        $column_list_row .= '|';

        // has person right to write / edit entries ?
        if (Security::hasRight($row['data_id'], 'write','website')) {
          $security = 'write';
        }

        // table data
        $tdata .= '<row id="'.$row['data_id2'].'"';
        $tdata .= ' dname="'.$row['data_name'].'"';
        $tdata .= ' type="'.$row['data_type'].'"';
        $tdata .= ' rid="'.$row['rev_id'].'"';
        $tdata .= ' rver="'.$row['rev_version'].'"';
        $tdata .= ' rlang="'.$row['rev_language'].'"';
        $tdata .= ' rdate="'.$row['rev_datetime'].'"';
        $tdata .= ' rusrid="'.$row['rev_usrid'].'"';
        $tdata .= ' star="'. $star_state .'"'; /* starred (on=1, off=0) */
        $tdata .= ' starid="'. $star_id .'"'; /* star tag id (from getTagId(...)) */
        $tdata .= ' status="'. $line_status .'"'; /* status (odd/even (=stable), new, draft, etc.) */
        $tdata .= ' security="'. $security ."\""; /* security (read, write, add, pub, trans) */
        $tdata .= ' xtrcol="'.$column_list_row."\"";
        $tdata .= '><![CDATA['.urlencode(substr($stext_content, 30)).']]></row>';

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
                $this->sql_order .= " r.rev_datetime ";
                break;
              case 'name': // name
                $this->sql_order .= "d.data_name ";
                break;
              case 'lang': // language
                $this->sql_order .= "r.rev_language ";
                break;
              case 'usr': // user
                $this->sql_order .= "r.rev_usrid ";
                break;
              case 'ver': // version
                $this->sql_order .= "r.rev_version ";
                break;
              case 'nbr': // number ("dynamic" entry)
                $tag_counter++;
                $this->sql_order .= " v".$tag_counter.".tv_value";
                $this->sql_where .= " AND n".$tag_counter.".tn_name = 'number_sort' ";
                break;
              case 'type': // type
                $this->sql_order .= "d.data_type ";
                break;
              case 'security': // security (ACL)
                $this->sql_order .= "d.data_acl ";
                break;
              case 'revid': // revision-id
                $this->sql_order .= "r.rev_id ";
                break;
              case 'ext': // page extension 
                $tag_counter++;
                $this->sql_order .= " v".$tag_counter.".tv_value ";
                $this->sql_where .= " AND n".$tag_counter.".tn_name = 'extension' ";
                break;
              case 'status': // status 
                $tag_counter++;
                $this->sql_order .= " v".$tag_counter.".tv_value ";
                $this->sql_where .= " AND n".$tag_counter.".tn_name = 'status' ";
                break;
              case 'kind': // kind 
                $tag_counter++;
                $this->sql_order .= " v".$tag_counter.".tv_value ";
                $this->sql_where .= " AND n".$tag_counter.".tn_name = 'kind' ";
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
            $this->a = "_a";
            $this->a2 = "a";
            break;
        }
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
            if ($type_c == "draft") {
              $entries_private++;
            }
            elseif ($type_c == "system") {
              $entries_system++;
            }
            else {
              $entries_public++;
            }

              // ask for status-tag value
              $tag_counter++;
              $this->sql_where .= " (n".$tag_counter.".tn_name = 'status' AND v".$tag_counter.".tv_value".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR).") ";
            break;

          // tag
          case 'a': 
            $tag_counter++;
            $this->sql_where .= " (n".$tag_counter.".tn_name = 'tag' AND v".$tag_counter.".tv_value".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR).") ";
            break;

          // entry name
          case 'n': 
            $this->sql_where .= "d.data_name";

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
            $this->sql_where .= "d.data_type".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
            break;

          // starred
          case 's': 
            $tag_counter++;
            $this->sql_where .= " (n".$tag_counter.".tn_name = 'star' AND v".$tag_counter.".tv_value = '".($type_c=='true'?'on':'off')."') ";
            break;

          // date
          case 'd': 
            $this->sql_where .= "r.rev_date";

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
            $this->sql_where .= "r.rev_time";

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
            $this->sql_where .= "r.rev_language".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
            break;

          // security (ACL)
          case 'i': 
            $this->sql_where .= "d.data_acl".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
            break;

          // metadata
          case 'm': 
            $tag_counter++;
            $this->sql_where .= " (n".$tag_counter.".tn_name = ".DBConnection::getInstance()->quote(strtolower($type_b),PDO::PARAM_STR)." AND v".$tag_counter.".tv_value = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR).") ";
            break;

          // user
          case 'u': 
            // get user_id
            $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM users WHERE user_name = :user_name LIMIT 1");
            $stmt->bindParam('user_name',$type_c);
            $stmt->execute();
            $user_id = $stmt->fetchColumn();

            $this->sql_where .= "r.rev_usrid".($type_b=='is' ? '=':'!=').DBConnection::getInstance()->quote($user_id,PDO::PARAM_INT);
            break;

          // version
          case 'v': 
            $this->sql_where .= "r.rev_version";

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
                $this->sql_where .= "r.rev_id = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
                break;
              case 'usrid':
                $this->sql_where .= "r.rev_usrid = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
                break;
              case 'langid':
                $this->sql_where .= "r.rev_language = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_STR);
                break;
              case 'dataid':
              default:
                $this->sql_where .= "d.data_id = ".DBConnection::getInstance()->quote($type_c,PDO::PARAM_INT);
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

      // everything except draft
      if ($thisuser->securityLevel() == 3) { 
        $this->sql_where .= " AND (n.tn_name = 'status' AND v.tv_value != 'draft') ";
      }

      // new, stable and unknown (if more than translator)
      if ($thisuser->securityLevel() == 2) { 
        $this->sql_where .= " AND (n.tn_name = 'status' AND (v.tv_value = 'new' OR v.tv_value = 'stable' OR v.tv_value = 'unknown')) ";
      }
      else {
        $this->sql_where .= " AND (n.tn_name = 'status' AND (v.tv_value = 'new' OR v.tv_value = 'stable')) ";
      }

      // set additional needed sql
      $this->sql_select .= ", n.tn_name, v.tv_value ";
      $this->sql_from .= ", data_tag".$this->a." a, data_tag_name".$this->a." n, data_tag_value".$this->a." v ";
      $this->sql_where .= " AND r.data_id = a.data_id AND r.rev_id = a.data_rev_id AND a.tag_usrid IN(-1, 0, ".DBConnection::getInstance()->quote($thisuser->id(),PDO::PARAM_INT).") AND a.tag_name_id = n.tn_id AND a.tag_value_id  = v.tv_id ";
    }

    // construct additioanl sql for tag-usage from filter
    if ($tag_counter > 0) {
      for ($i = 1; $i <= $tag_counter; $i++) {
        $this->sql_select .= ", n".$i.".tn_name, v".$i.".tv_value ";
        $this->sql_from .= ", data_tag".$this->a." a".$i.", data_tag_name".$this->a." n".$i.", data_tag_value".$this->a." v".$i." ";
        $this->sql_where .= " AND r.data_id = a".$i.".data_id AND r.rev_id = a".$i.".data_rev_id AND (a".$i.".tag_usrid = '-1' OR a".$i.".tag_usrid = '0' OR a".$i.".tag_usrid = ".DBConnection::getInstance()->quote($thisuser->id(),PDO::PARAM_INT).") AND a".$i.".tag_name_id = n".$i.".tn_id AND a".$i.".tag_value_id  = v".$i.".tv_id ";
      }
    }

    // make sure only private drafts are visible
    if ($thisuser->securityLevel() < 3 && $entries_private > 0) {
      $this->sql_where .= " AND r.rev_usrid = '".$thisuser->id()."' ";
    }

    // either show draft (private) OR stable & new (public) entries,   private AND public entries together are NOT allowed => block 
    if ($thisuser->securityLevel() < 2 && (($entries_private > 0 && $entries_public > 0) || $entries_system > 0)) {
      $this->sql_select = "";
      $this->sql_from   = "";
      $this->sql_where  = " FALSE ";
      $this->sql_order  = "";
    }

  } // end of member function generateFilterSQL

} // end of Export_XML
?>
