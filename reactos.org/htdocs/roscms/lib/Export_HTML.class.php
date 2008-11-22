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
 * class Export_HTML
 * 
 */
class Export_HTML extends Export
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/
   
  private $lang = '';

  private $dynamic_num = 0;

  private $output_type = '';

  private $page_name = '';



  /**
   * 
   *
   * @return 
   * @access public
   */
  public function generate( $data_id, $lang, $dynamic_num = 0 )
  {
    global $roscms_standard_language;

    // get content (prefered in the given language, otherwise standard language)
    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_type, d.data_name, d.data_id, r.rev_id, r.rev_language FROM data_ d JOIN data_revision r ON r.data_id = d.data_id WHERE r.data_id = :data_id AND (r.rev_language = :lang_one OR r.rev_language = :lang_two) AND rev_version > 0 LIMIT 2");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('lang_one',$lang,PDO::PARAM_STR);
    $stmt->bindParam('lang_two',$roscms_standard_language,PDO::PARAM_STR);
    $stmt->execute();
    $results=$stmt->fetchAll(PDO::FETCH_ASSOC);
    $data = @$results[0];

    // try to get the dataset with rev_language == $lang, to boost the translated content
    if( count($results) == 2 && $data['rev_language'] == $roscms_standard_language ) {
      $data = $results[1];
    }

    //@ADD doku here
    if ($lang == $roscms_standard_language) {
      $tmp_lang = 'all';
    }
    else {
      $tmp_lang = $lang;
    }
    Log::writeGenerateLow('-=&gt; '.$data['data_name'].' ('.$tmp_lang.') type: '.$data['data_type'].' [generate_page_output_update('.$data_id.', '.$lang.', '.$dynamic_num.')]');

    //
    switch ($data['data_type']) {
      case 'page':
        Log::writeGenerateLow($data['data_name'].' ('.$tmp_lang.')  type: '.$data['data_type'].' '.$dynamic_num);
        Log::writeGenerateMedium($data['data_name'].' ('.$tmp_lang.') '.$dynamic_num);
        $this->generateFiles($data['data_name'], $tmp_lang, $dynamic_num);
        break;

      case 'template':
        self::handleDepencies($tmp_lang, $data['data_type'], $data['data_name']);
        break;

      case 'content':
        // get dynamic content number
        $tmp_dynamic = Tag::getValueByUser($data['data_id'], $data['rev_id'], 'number', -1);
        if ($tmp_dynamic > 0) {
          Log::writeGenerateGow($data['data_name'].' ('.$tmp_lang.')  type: '.$data['data_type'].' '.$dynamic_num);
          Log::writeGenerateMedium($data['data_name'].' ('.$tmp_lang.') '.$dynamic_num);
          $this->generateFiles($data['data_name'], $tmp_lang, $dynamic_num);
        }
        else {
          self::handleDepencies($tmp_lang, $data['data_type'], $data['data_name']);
        }
        break;

      case 'script':
        self::handleDepencies($tmp_lang, $data['data_type'], $data['data_name']);
        break;

      // do nothing
      case 'system':
      default:
        break;
    }
  } // end of member function generate_page_output_update


  /**
   * 
   *
   * @return 
   * @access public
   */
  public function generateFiles($page_name, $lang_short='', $dynamic_num = 0, $mode = 'single')
  {
    global $roscms_standard_language;

    global $roscms_extern_brand;
    global $roscms_extern_version;
    global $roscms_extern_version_detail;

    $destination_folder = '../'; // distance between roscms folder to pages folder

    // do for all or only one language
    if ($lang_short == 'all') {
      $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM languages ORDER BY lang_id ASC");
    }
    else {
      $stmt=DBConnection::getInstance()->prepare("SELECT lang_id, lang_name FROM languages WHERE lang_id = :lang LIMIT 1");
      $stmt->bindParam('lang',$lang_short,PDO::PARAM_STR);
    }
    $stmt->execute();
    while ($lang = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // display language
      echo '<p><b><u>'.$lang['lang_name'].'</u></b></p>';

      // switch sql statement by generation mode
      if ($mode == 'single') {
        $stmt_page=DBConnection::getInstance()->prepare("SELECT d.data_name, r.data_id, r.rev_id, r.rev_language FROM data_ d, data_revision r WHERE data_type = 'page' AND r.data_id = d.data_id AND r.rev_version > 0 AND (r.rev_language = :lang_one OR r.rev_language = :lang_two)  AND data_name = :data_name ORDER BY r.rev_version DESC LIMIT 1");
        $stmt_page->bindParam('data_name',$page_name,PDO::PARAM_STR);
      }
      else {
        $stmt_page=DBConnection::getInstance()->prepare("SELECT d.data_name, r.data_id, r.rev_id, r.rev_language FROM data_ d JOIN data_revision r ON r.data_id = d.data_id WHERE data_type = 'page' AND r.rev_version > 0 AND (r.rev_language = :lang_one OR r.rev_language = :lang_two) ORDER BY r.rev_version DESC ");
      }
      $stmt_page->bindParam('lang_one',$lang['lang_id'],PDO::PARAM_STR);
      $stmt_page->bindParam('lang_two',$roscms_standard_language,PDO::PARAM_STR);
      $stmt_page->execute();
      while ($page = $stmt_page->fetch(PDO::FETCH_ASSOC)) {

        // distinguish between dynamic and non dynamic driven pages
        $kind_of_content = Tag::getValueByUser($page['data_id'], $page['rev_id'], 'kind', -1);
        if ($kind_of_content == 'dynamic' && $dynamic_num == '') {
          $stmt_content=DBConnection::getInstance()->prepare("SELECT r.rev_id, r.rev_version, r.rev_usrid, r.rev_datetime, r.rev_date, r.rev_time, v.tv_value FROM data_ d JOIN data_revision r ON r.data_id = d.data_id JOIN data_tag a ON (r.data_id = a.data_id AND r.rev_id = a.data_rev_id) JOIN data_tag_name n ON a.tag_name_id = n.tn_id JOIN data_tag_value v ON a.tag_value_id  = v.tv_id WHERE d.data_name = :name AND d.data_type = 'content' AND r.rev_version > 0 AND (r.rev_language = :lang_one OR r.rev_language = :lang_two) AND a.tag_usrid = '-1' AND n.tn_name = 'number' ORDER BY v.tv_value ASC");
          $stmt_content->bindParam('name',$page['data_name'],PDO::PARAM_STR);
          $stmt_content->bindParam('lang_one',$lang['lang_id'],PDO::PARAM_STR);
          $stmt_content->bindParam('lang_two',$roscms_standard_language,PDO::PARAM_STR);
          $stmt_content->execute();
        }
        else {
          $stmt_content=DBConnection::getInstance()->prepare("SELECT 1 LIMIT 1");
        }
        $stmt_content->execute();
        while ($content = $stmt_content->fetch(PDO::FETCH_ASSOC)) {

          // get dynamic number for content
          $is_dynamic = (Tag::getValueByUser($page['data_id'], $page['rev_id'], 'kind', -1)=='dynamic'); 
          if ($is_dynamic && $dynamic_num == '') {
            $content['dynamic_num'] = $content['tv_value'];
          }
          elseif ($is_dynamic == 'dynamic' && $dynamic_num > 0) {
            $content['dynamic_num'] = $dynamic_num;
          }
          else {
            $content['dynamic_num'] = '';
          }

          // check file extension
          $file_extension = Tag::getValueByUser($page['data_id'], $page['rev_id'], 'extension', -1);
          if ($file_extension == '') {
            echo '<p><strong>!! '.date('Y-m-d H:i:s').' - file extension missing: '.$page['data_name'].'('.$page['data_id'].', '.$page['rev_id'].', '.$lang['lang_id'].')</strong></p>';
            continue;
          }

          // get file name & content
          $file_name = $lang['lang_id'].'/'.$page['data_name'].($is_dynamic ? '_'.$content['dynamic_num'] : '').'.'.$file_extension;
          $html_content = $this->processTextByName($page['data_name'], $lang['lang_id'], $content['dynamic_num'], 'output');

          // write content to filename, if possible
          $fh = @fopen($destination_folder.$file_name, "w");
          if ($fh!==false){
            flock($fh,2);
            fputs($fh,$html_content); 
            fputs($fh,"\n\n<!-- Generated with ".$roscms_extern_brand." ".$roscms_extern_version.' ('.$roscms_extern_version_detail.') - '.date('Y-m-d H:i:s').' [RosCMS_v3] -->');
            flock($fh,3);
            fclose($fh);
            
            echo '<p> * '.date('Y-m-d H:i:s').' - "'.$file_name.'"</p>';
          }
          else{
            echo '<p>* could not create file: "'.$destination_folder.$file_name.'"</p>';
          }

        } // content while
      } // page while
    } // lang while
  } // end of member function generate_page_output_update


  /**
   * replaces
   *
   * @return 
   * @access private
   */
  private function handleDepencies($lang, $data_type, $like_phrase)
  {
    global $roscms_standard_language;

    Log::writeGenerateLow('-=&copy; '.$lang.' ('.$like_phrase.') type: '.$data_type);

    switch ($data_type) {
      case 'template':
        $type_short = 'templ';
        $sql_type = "";
        break;	
      case 'content':
        $type_short = 'cont';
        $sql_type = " OR d.data_type = 'template' ";
        break;	
      case 'script':
        $type_short = 'inc';
        $sql_type = " OR d.data_type = 'template' OR d.data_type = 'content' OR d.data_type = 'script' ";
        break;
      default:
        die('should never happen: generate_update_helper('.$data_type.', '.$like_phrase.')');
        break;
    }

    // search for depencies
    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_name, d.data_type, r.data_id, r.rev_id, r.rev_language FROM data_ d, data_revision r JOIN data_text t ON r.data_id = d.data_id WHERE (d.data_type = 'page' ".$sql_type." ) AND r.rev_id = t.data_rev_id AND t.text_content  LIKE :content_phrase AND r.rev_language = :lang AND r.rev_version > 0");
    $stmt->bindParam('lang',$lang);
    $stmt->bindValue('content_phrase','%[#'.$type_short.'_'.$like_phrase.']%',PDO::PARAM_STR);
    $stmt->execute();

    // handle Depencies
    while ($data = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // generate for all or selected language
      if ($data['rev_language'] == $roscms_standard_language) {
        $lang = 'all';
      }
      else {
        $lang = $data['rev_language'];
      }

      // generate Page
      if ($data['data_type'] == 'page') {
        Log::writeGenerateLow($data['data_name'].' ('.$lang.') type: '.$data['data_type']);
        Log::writeGenerateMedium($data['data_name'].' ('.$lang.') ');
        $this->generateFiles($data['data_name'], $lang);
      }
      else {
        // get dynamic content number
        $dynamic_num = Tag::getValueByUser($data['data_id'], $data['rev_id'], 'number', -1);
        if ($dynamic_num > 0 && $data['data_type'] == 'content') {
          Log::writeGenerateLow($data['data_name'].' ('.$lang.') type: '.$data['data_type'].' '.$dynamic_num);
          Log::writeGenerateMedium($data['data_name'].' ('.$tmp_lang.') '.$dynamic_num);
          $this->generateFiles($data['data_name'], $lang, $dynamic_num);
        }
        self::handleDepencies($lang, $data['data_type'], $data['data_name']);
      }
    } // while
  } // end of member function generate_page_output_update


  /**
   * processes the content
   * need to call this in object context
   *
   * @return content
   * @access public
   */
  public function processTextByName( $page_name, $lang = '', $dynamic_num = 0, $output_type = '' )
  {
    global $roscms_standard_language;

    // use class vars instead of give them every method as param
    $this->page_name = $page_name;
    $this->lang = $lang;
    $this->dynamic_num = intval($dynamic_num);
    $this->output_type = $output_type;

    //
    if ($dynamic_num > 0) {
      $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id, r.rev_language FROM data_ d JOIN data_revision r ON r.data_id = d.data_id JOIN data_tag t ON t.data_rev_id=r.rev_id JOIN data_tag_name n ON t.tag_name_id=n.tn_id JOIN data_tag_value v ON t.tag_value_id=v.tv_id WHERE data_name = :name AND data_type = 'page' AND r.rev_version > 0 AND (r.rev_language = :lang_one OR r.rev_language = :lang_two) AND n.tn_name = 'number' AND t.tag_usrid = '-1' AND v.tv_value = :dynamic_num ORDER BY r.rev_version DESC LIMIT 2");
      $stmt->bindParam('dynamic_num',$dynamic_num,PDO::PARAM_INT);
    }
    else {
      $stmt=DBConnection::getInstance()->prepare("SELECT r.rev_id, r.rev_language FROM data_ d JOIN data_revision r ON r.data_id = d.data_id WHERE data_name = :name AND data_type = 'page' AND r.rev_version > 0 AND (r.rev_language = :lang_one OR r.rev_language = :lang_two) ORDER BY r.rev_version DESC LIMIT 2");
    }
    $stmt->bindParam('name',$page_name,PDO::PARAM_STR);
    $stmt->bindParam('lang_one',$lang,PDO::PARAM_STR);
    $stmt->bindParam('lang_two',$roscms_standard_language,PDO::PARAM_STR);
    $stmt->execute();
    $results=$stmt->fetchAll(PDO::FETCH_ASSOC);
    $page = @$results[0];

    // try to get the dataset with rev_language == $lang, to boost the translated content
    if( count($results) == 2 && $page['rev_language'] == $roscms_standard_language ) {
      $page = $results[1];
    }

    $this->processText($page['rev_id'], $output_type);
  }


  /**
   * processes the content
   * need to call this in object context
   *
   * @return content
   * @access public
   */
  public function processText( $rev_id, $output_type = '' )
  {
    global $roscms_intern_account_id;
    global $roscms_standard_language_full;
    global $roscms_intern_webserver_pages;
    global $roscms_intern_webserver_roscms;
    global $roscms_standard_language;
    global $roscms_subsystem_wiki_path;

    // try to force unlimited script runtime
    @set_time_limit(0);

    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_name, d.data_id, r.rev_id, r.rev_version, r.rev_usrid, r.rev_datetime, r.rev_date, r.rev_time, r.rev_language FROM data_ d JOIN data_revision r ON r.data_id = d.data_id WHERE r.rev_id = :rev_id ORDER BY r.rev_version DESC LIMIT 2");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $page=$stmt->fetch(PDO::FETCH_ASSOC);

    if ($page['rev_id'] === false) {
      return; // nothing found
    }

    $this->page_name = $page['data_name'];
    $this->lang = $page['rev_language'];
    $this->dynamic_num = Tag::getValueByUser($page['data_id'], $page['rev_id'], 'number', -1);
    $this->output_type = $output_type;
    // replace dynamic tags first and then static

    // get content and replace with other contents
    $content = Data::getText($page['rev_id'], 'content');
    $content = preg_replace_callback('/(\[#templ_[^][#[:space:]]+\])/', array($this,'insertTemplate'), $content);
    $content = preg_replace_callback('/(\[#cont_[^][#[:space:]]+\])/', array($this,'insertContent'), $content);
    $content = preg_replace_callback('/(\[#inc_[^][#[:space:]]+\])/', array($this,'insertScript'), $content);
    $content = preg_replace_callback('/(\[#link_[^][#[:space:]]+\])/', array($this,'insertHyperlink'), $content);

    // website url
    $content = str_replace('[#roscms_path_homepage]', $roscms_intern_webserver_pages, $content);

    // replace roscms constants dependent from dynamic number
    if ($this->dynamic_num > 0) {
      $content = str_replace('[#roscms_filename]', $this->page_name.'_'.$this->dynamic_num.'.html', $content);
      $content = str_replace('[#roscms_pagename]', $this->page_name.'_'.$this->dynamic_num, $content); 
      $content = str_replace('[#roscms_pagetitle]', ucfirst(Data::getSText($page['rev_id'], 'title')).' #'.$this->dynamic_num, $content); 
    }
    // take care of dynamic number independent entries
    else {
      $content = str_replace('[#roscms_filename]', $this->page_name.'.html', $content); 
      $content = str_replace('[#roscms_pagename]', $this->page_name, $content); 
      $content = str_replace('[#roscms_pagetitle]', ucfirst(Data::getSText($page['rev_id'], 'title')), $content); 
    }

    // replace current language stuff
    $stmt=DBConnection::getInstance()->prepare("SELECT lang_name FROM languages WHERE lang_id = :lang LIMIT 1");
    $stmt->bindParam('lang',$this->lang);
    $stmt->execute();
    $content = str_replace('[#roscms_language]', $stmt->fetchColumn(), $content); 
    $content = str_replace('[#roscms_language_short]', $this->lang, $content); 

    // @REMOVEME if it is no more present in Database
    $content = str_replace('[#roscms_format]', 'html', $content); 

    // replace date and time
    $content = str_replace('[#roscms_date]', date('Y-m-d'), $content); 
    $localtime = localtime(time() , 1);
    $content = str_replace('[#roscms_time]', sprintf('%02d:%02d', $localtime['tm_hour'],$localtime['tm_min']), $content);

    // replace with user_name
    // @FIXME broken logic, or one link too much, which should be removed from Database
    $stmt=DBConnection::getInstance()->prepare("SELECT user_name FROM users WHERE user_id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',$roscms_intern_account_id);
    $stmt->execute();
    $user_name = $stmt->fetchColumn();
    $content = str_replace('[#roscms_user]', $user_name, $content); // account that generate
    $content = str_replace("[#roscms_inc_author]", $user_name, $content); // account that changed the include text

    // page version
    $content = str_replace('[#roscms_page_version]', $page['rev_version'], $content); 

    // page edit link
    $content = str_replace('[#roscms_page_edit]', $roscms_intern_webserver_roscms.'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$this->page_name.'&amp;d_val2='.$this->lang.'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit', $content);

    // Subsystem Links
    $content = preg_replace('/\[#wiki_([^]]+)\]/', $roscms_subsystem_wiki_path.'$1', $content);

    // translation info
    if ($this->lang == $roscms_standard_language) {
      $content = str_replace('[#roscms_trans]', '<p><a href="'.$roscms_intern_webserver_roscms.'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$this->page_name.'&amp;d_val2='.$this->lang.'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit" style="font-size:10px !important;">Edit page content</a> (RosCMS translator account membership required, visit the <a href="'.$roscms_intern_webserver_pages.'forum/" style="font-size:10px !important;">website forum</a> for help)</p><br />', $content); 
    }
    else {
      $content = str_replace('[#roscms_trans]', '<p><em>If the translation of the <a href="'.$roscms_intern_webserver_pages.'?page='.$this->page_name.'&amp;lang='.$roscms_standard_language.'" style="font-size:10px !important;">'.$roscms_standard_language_full.' language</a> of this page appears to be outdated or incorrect, please check-out the <a href="'.$roscms_intern_webserver_pages.'?page='.$this->page_name.'&amp;lang='.$roscms_standard_language.'" style="font-size:10px !important;">'.$roscms_standard_language_full.'</a> page and <a href="'.$roscms_intern_webserver_pages.'forum/viewforum.php?f=18" style="font-size:10px !important;">report</a> or <a href="'.$roscms_intern_webserver_roscms.'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$this->page_name.'&amp;d_val2='.$this->lang.'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit" style="font-size:10px !important;">update the content</a>.</em></p><br />', $content); 
    }

    return $content;
  } // end of member function generate_content


  /**
   * replace [#link_ABC] with roscms link to ABC
   *
   * @param string[] matches

   * @return 
   * @access private
   */
  private function insertContent($matches)
  {

    // extract the name, e.g. [#cont_about] ... "about"
    $content_name = substr($matches[0], 7, (strlen($matches[0])-8)); 

    // get new content and return it
    return $this->sharedInsertHelper('content', $content_name, $this->lang);
  } // end of member function generate_content


  /**
   * replace [#link_ABC] with roscms link to ABC
   *
   * @param string[] matches

   * @return 
   * @access private
   */
  private function insertScript($matches)
  {

    // extract the name, e.g. [#inc_about] ... "about"
    $content_name = substr($matches[0], 6, (strlen($matches[0])-7)); 

    return $this->sharedInsertHelper('script', $content_name, $this->lang);
  } // end of member function generate_content


  /**
   * replace [#link_ABC] with roscms link to ABC
   *
   * @param string[] matches

   * @return 
   * @access private
   */
  private function insertHyperlink( $matches )
  {
    global $roscms_intern_account_id;
    global $roscms_intern_webserver_pages;
    global $roscms_intern_webserver_roscms;

    global $RosCMS_GET_d_value4;

    //@CHANGE get rid of that global var
    $mode = $RosCMS_GET_d_value4;
    $page_name = substr($matches[0], 7, (strlen($matches[0])-8)); 

    $page_name_with_num = $page_name; // page name without number (if exists)
    $dynamic_num = null; // number in page name

    // is dynamic page ?
    if ( preg_match('/^(.*)_([1-9][0-9]*)$/', $page_name, $matches) ) {
      $page_name_with_num = $matches[1];
      $dynamic_num = $matches[2];
    }

    // preview
    if ($this->output_type == 'show') {

      //
      if ($page_name == '') {
        $page_link = $roscms_intern_webserver_roscms.'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=index&amp;d_val2='.$this->lang.'&amp;d_val3=';
      }
      else {
        $page_link = $roscms_intern_webserver_roscms."?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=".$page_name_with_num."&amp;d_val2=".$this->lang."&amp;d_val3=".$dynamic_num;
      }

      if ($mode == 'edit') {
        $page_link .= '&amp;d_val4=edit';
      }
    }

    // static pages
    else { 
      $stmt=DBConnection::getInstance()->prepare("SELECT r.data_id, r.rev_id FROM data_ d JOIN data_revision r ON r.data_id = d.data_id WHERE d.data_name  = :name AND r.rev_language = :lang AND rev_version > 0 LIMIT 1");
      $stmt->bindParam('name',$page_name,PDO::PARAM_STR);
      $stmt->bindParam('lang',$this->lang,PDO::PARAM_STR);
      $stmt->execute();
      $data = $stmt->fetchOnce(PDO::FETCH_ASSOC);

      // get page extension
      $link_extension = Tag::getValueByUser($data['data_id'], $data['rev_id'], 'extension', -1);
      if ($link_extension == '') {
        $link_extension = 'html';
      }

      // get page link
      if ($g_link_page_name == '') {
        $page_link = $roscms_intern_webserver_pages.$this->lang.'/404.html';
      }
      else{
        $page_link = $roscms_intern_webserver_pages.$this->lang.'/'.$page_name.'.'.$link_extension;
      }
    }

    return $page_link;
  } // end of member function replace_hyperlink



  /**
   * 
   *
   * @param string[] matches 

   * @return 
   * @access private
   */
  private function sharedInsertHelper( $match_type, $match_name, $lang )
  {
    global $roscms_intern_webserver_roscms;
    global $roscms_intern_page_link;
    global $roscms_standard_language;

    global $RosCMS_GET_d_flag;

    // map them until the global vars removed
    // it may be eventually d_flag / d_value4, was used both in same context
    $mode =$RosCMS_GET_d_flag;

    // get entry
    $stmt=DBConnection::getInstance()->prepare("SELECT d.data_acl, t.text_content, r.rev_version, r.rev_usrid, r.rev_datetime , r.data_id, r.rev_id, r.rev_language FROM data_ d JOIN data_revision r ON r.data_id = d.data_id JOIN data_text t ON t.data_rev_id = r.rev_id WHERE data_name = :name AND data_type = :type AND r.rev_version > 0 AND (r.rev_language = :lang_one OR r.rev_language = :lang_two) AND t.text_name = 'content' ORDER BY r.rev_version DESC LIMIT 2");
    $stmt->bindParam('name',$match_name,PDO::PARAM_STR);
    $stmt->bindParam('type',$match_type,PDO::PARAM_STR);
    $stmt->bindParam('lang_one',$lang,PDO::PARAM_STR);
    $stmt->bindParam('lang_two',$roscms_standard_language,PDO::PARAM_STR);
    $stmt->execute();
    $results=$stmt->fetchAll();

    // check if required content is available
    // depency not available
    if (count($results)==0){
      echo '<p>* <strong>Not found ('.$match_type.': '.$match_name.')</strong></p>';
      return;
    }

    // try to get the dataset with rev_language == $lang, to boost the translated content
    $content = @$results[0];
    if (count($results) == 2 && $content['rev_language'] == $roscms_standard_language ) {
      $content = $results[1];
    }

    // get basicly content
    $html_content = $content['text_content'];

    // execute script and get echoed content
    if ($match_type == 'script' && Tag::getValue($content['data_id'], $content['rev_id'], 'kind') == 'php') {
      $html_content = $this->evalTemplate($html_content, $this->dynamic_num, $lang);
    }

    // text for username
    $stmt=DBConnection::getInstance()->prepare("SELECT user_name, user_fullname FROM users WHERE user_id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',$content['rev_usrid'],PDO::PARAM_INT);
    $stmt->execute();
    $user_account = $stmt->fetchOnce(PDO::FETCH_ASSOC);
    if ($user_account['user_fullname']) {
      $user_text = $user_account['user_fullname'].' ('.$user_account['user_name'].')';
    }
    else {
      $user_text = $user_account['user_name'];
    }

    // last modified ... by ...
    $html_content = str_replace('[#roscms_'.$match_type.'_version]', '<em>Last modified: '.$content['rev_datetime'].', rev. '.$content['rev_version'].' by '.$user_text.'</em>', $html_content); 

    // preview-edit-mode (add some html through it)
    if ($mode == 'edit' && $content['data_acl'] == 'default' && $match_type != 'script') {
      $html_content = '<div style="border: 1px dashed red;"><div style="padding: 2px;"><a href="'.$roscms_intern_page_link.'data&amp;branch='.(isset($_GET['branch'])?htmlentities($_GET['branch']):RCMS_STD_BRANCH).'&amp;edit=rv'.$content['data_id'].'|'.$content['rev_id'].'" style="background-color:#E8E8E8;"> <img src="'.$roscms_intern_webserver_roscms.'images/edit.gif" style="width:19px; height:19px; border:none;" /><em>'.$match_name.'</em> </a></div>'.$html_content.'</div>';
    }

    return $html_content;
  } // end of member function replace_hyperlink


  /**
   * 
   *
   * @param string[] matches 

   * @return 
   * @access private
   */
  private function insertTemplate( $matches )
  {
    global $roscms_intern_account_id;

    // extract the name, e.g. [#templ_about] -> 'about'
    $content_name = substr($matches[0], 8, (strlen($matches[0])-9)); 

    // get content and replace stuff
    $new_content = $this->sharedInsertHelper('template', $content_name, $this->lang);
    $new_content = str_replace("[#cont_%NAME%]", "[#cont_".$this->page_name.']', $new_content); 
    $new_content = str_replace("[#%NAME%]", $this->page_name, $new_content); 

    return $new_content;
  } // end of member function insert_template



  /**
   * executes code from a script
   *
   * @param string code 
   
   * @param int dynamic_num the dynamic changing number from e.g. a newsletter
   
   * @param string lang 

   * @return 
   * @access private
   */
  private function evalTemplate( $code, $dynamic_num, $language )
  {
    // catch output
    ob_start(); 

    // some vars for template usage;
    //@RENAMEME or better remove me, parmeter names should be enough
    $roscms_template_var_pageid = $dynamic_num;
    $roscms_template_var_lang = $language;

    // execute code and return the output
    eval(' ?> '.$code.' <?php ');
    $output = ob_get_contents(); 
    ob_end_clean(); 
    return $output; 
  } // end of member function eval_template



} // end of AJAX
?>
