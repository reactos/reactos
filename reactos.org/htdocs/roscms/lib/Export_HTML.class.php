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
  public function generate( $data_id, $lang_id, $dynamic_num = 0 )
  {
    // get content (prefered in the given language, otherwise standard language)
    $stmt=&DBConnection::getInstance()->prepare("SELECT d.type, d.name, r.data_id, r.lang_id, r.id, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE d.id = :data_id AND r.lang_id IN( :lang_one, :lang_two) AND r.version > 0 LIMIT 2");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('lang_one',$lang_id,PDO::PARAM_INT);
    $stmt->bindParam('lang_two',Language::getStandardId(),PDO::PARAM_INT);
    $stmt->execute();
    $results=$stmt->fetchAll(PDO::FETCH_ASSOC);
    $data = @$results[0];

    // try to get the dataset with rev_language == $lang, to boost the translated content
    if( count($results) == 2 && $data['lang_id'] == Language::getStandardId() ) {
      $data = $results[1];
    }

    //@ADD doku here
    if ($lang_id == Language::getStandardId()) {
      $tmp_lang = 'all';
    }
    else {
      $tmp_lang = $lang_id;
    }
    Log::writeGenerateLow('-=&gt; '.$data['name'].' ('.$tmp_lang.') type: '.$data['type'].' [generate_page_output_update('.$data_id.', '.$lang.', '.$dynamic_num.')]');

    //
    switch ($data['type']) {
      case 'page':
        Log::writeGenerateLow($data['name'].' ('.$tmp_lang.')  type: '.$data['type'].' '.$dynamic_num);
        Log::writeGenerateMedium($data['name'].' ('.$tmp_lang.') '.$dynamic_num);
        $this->generateFiles($data['name'], $tmp_lang, $dynamic_num);
        break;

      case 'template':
        self::handleDepencies($tmp_lang, $data['type'], $data['name']);
        break;

      case 'content':
        // get dynamic content number
        $tmp_dynamic = Tag::getValueByUser($data['id'], 'number', -1);
        if ($tmp_dynamic > 0) {
          Log::writeGenerateGow($data['name'].' ('.$tmp_lang.')  type: '.$data['type'].' '.$dynamic_num);
          Log::writeGenerateMedium($data['name'].' ('.$tmp_lang.') '.$dynamic_num);
          $this->generateFiles($data['name'], $tmp_lang, $dynamic_num);
        }
        else {
          self::handleDepencies($tmp_lang, $data['type'], $data['name']);
        }
        break;

      case 'script':
        self::handleDepencies($tmp_lang, $data['type'], $data['name']);
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
  public function generateFiles( $page_name, $lang_id = 0, $dynamic_num = 0, $mode = 'single' )
  {
    $destination_folder = '../'; // distance between roscms folder to pages folder

    // do for all or only one language
    if ($lang_id === 0) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, name_short, name FROM ".ROSCMST_LANGUAGES." ORDER BY name ASC");
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, name_short, name FROM ".ROSCMST_LANGUAGES." WHERE id = :lang LIMIT 1");
      $stmt->bindParam('lang',$lang_id,PDO::PARAM_STR);
    }
    $stmt->execute();
    while ($lang = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // display language
      echo '<h3 style="text-decoration:underlien;>'.$lang['name'].'</h3>';

      // switch sql statement by generation mode
      if ($mode == 'single') {
        $stmt_page=&DBConnection::getInstance()->prepare("SELECT d.name, r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id WHERE d.type = 'page' AND r.version > 0 AND r.lang_id IN(:lang_one, :lang_two) AND d.name = :data_name ORDER BY r.version DESC LIMIT 1");
        $stmt_page->bindParam('data_name',$page_name,PDO::PARAM_STR);
      }
      else {
        $stmt_page=&DBConnection::getInstance()->prepare("SELECT d.name, r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON d.id = r.data_id WHERE d.type = 'page' AND r.version > 0 AND r.lang_id IN(:lang_one, r.rev_language) ORDER BY r.version DESC");
      }
      $stmt_page->bindParam('lang_one',$lang['lang_id'],PDO::PARAM_INT);
      $stmt_page->bindParam('lang_two',Language::getStandardId(),PDO::PARAM_INT);
      $stmt_page->execute();
      while ($page = $stmt_page->fetch(PDO::FETCH_ASSOC)) {

        // distinguish between dynamic and non dynamic driven pages
        $kind_of_content = Tag::getValueByUser($page['id'], 'kind', -1);
        if ($kind_of_content == 'dynamic' && $dynamic_num == '') {
          $stmt_content=&DBConnection::getInstance()->prepare("SELECT r.id, r.version, r.user_id, r.datetime, t.value AS dynamic_num FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_TAGS." t WHERE d.name = :name AND d.type = 'content' AND r.version > 0 AND t.user_id = -1 AND t.name = 'number' ORDER BY t.value ASC");
          $stmt_content->bindParam('name',$page['name'],PDO::PARAM_STR);
          $stmt_content->execute();
        }
        else {
          // generate once
          $stmt_content=&DBConnection::getInstance()->prepare("SELECT 1 LIMIT 1");
        }
        $stmt_content->execute();
        while ($content = $stmt_content->fetch(PDO::FETCH_ASSOC)) {

          // get dynamic number for content
          $is_dynamic = (Tag::getValueByUser($page['id'], 'kind', -1)=='dynamic'); 
          if ($is_dynamic && $dynamic_num == 0) {
            // number is already ok
          }
          elseif ($is_dynamic && $dynamic_num > 0) {
            $content['dynamic_num'] = $dynamic_num;
          }
          else {
            $content['dynamic_num'] = '';
          }

          // check file extension
          $file_extension = Tag::getValueByUser($page['id'], 'extension', -1);
          if ($file_extension == '') {
            echo '<p><strong>!! '.date('Y-m-d H:i:s').' - file extension missing: '.$page['name'].'('.$page['id'].', '.$lang['name'].')</strong></p>';
            continue;
          }

          // get file name & content
          $file_name = $lang['name_short'].'/'.$page['name'].($is_dynamic ? '_'.$content['dynamic_num'] : '').'.'.$file_extension;
          $html_content = $this->processTextByName($page['name'], $lang['id'], $content['dynamic_num'], 'output');

          // write content to filename, if possible
          $fh = @fopen($destination_folder.$file_name, 'w');
          if ($fh !== false){
            flock($fh,2);
            fputs($fh,$html_content); 
            fputs($fh,'<!-- Generated with '.RosCMS::getInstance->systemBrand().' ('.RosCMS::getInstance->systemVersion().') - '.date('Y-m-d H:i:s').' [RosCMS_v3] -->');
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
  private function handleDepencies($lang_id, $data_type, $like_phrase)
  {
    Log::writeGenerateLow('-=&copy; '.$lang_id.' ('.$like_phrase.') type: '.$data_type);

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
    $stmt=&DBConnection::getInstance()->prepare("SELECT d.data_name, d.data_type, r.rev_id, r.rev_language FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_STEXT." t ON r.id = t.rev_id WHERE (d.data_type = 'page' ".$sql_type." ) AND t.content LIKE :content_phrase AND r.lang_id = :lang AND r.version > 0");
    $stmt->bindParam('lang',$lang_id);
    $stmt->bindValue('content_phrase','%[#'.$type_short.'_'.$like_phrase.']%',PDO::PARAM_STR);
    $stmt->execute();

    // handle Depencies
    while ($data = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // generate for all or selected language
      if ($data['lang_id'] == Language::getStandardId()) {
        $lang = 0; // all
      }
      else {
        $lang = $data['lang_id'];
      }

      // generate Page
      if ($data['type'] == 'page') {
        Log::writeGenerateLow($data['name'].' ('.$lang.') type: '.$data['type']);
        Log::writeGenerateMedium($data['name'].' ('.$lang.') ');
        $this->generateFiles($data['name'], $lang);
      }
      else {
        // get dynamic content number
        $dynamic_num = Tag::getValueByUser($data['id'], 'number', -1);
        if ($dynamic_num > 0 && $data['type'] == 'content') {
          Log::writeGenerateLow($data['name'].' ('.$lang.') type: '.$data['type'].' '.$dynamic_num);
          Log::writeGenerateMedium($data['name'].' ('.$tmp_lang.') '.$dynamic_num);
          $this->generateFiles($data['name'], $lang, $dynamic_num);
        }
        self::handleDepencies($lang, $data['type'], $data['name']);
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
  public function processTextByName( $page_name, $lang_id , $dynamic_num = 0, $output_type = '' )
  {
    //
    if ($dynamic_num > 0) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_TAGS." t WHERE d.name = :name AND d.type = 'page' AND r.version > 0 AND r.lang_id IN(:lang_one,:lang_two) AND t.name = 'number' AND t.user_id = -1 AND t.value = :dynamic_num ORDER BY r.version DESC LIMIT 2");
      $stmt->bindParam('dynamic_num',$dynamic_num,PDO::PARAM_INT);
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE d.name = :name AND d.type = 'page' AND r.version > 0 AND r.lang_id IN(:lang_one, :lang_two) ORDER BY r.version DESC LIMIT 2");
    }
    $stmt->bindParam('name',$page_name,PDO::PARAM_STR);
    $stmt->bindParam('lang_one',$lang_id,PDO::PARAM_INT);
    $stmt->bindParam('lang_two',Language::getStandardId(),PDO::PARAM_INT);
    $stmt->execute();
    $results=$stmt->fetchAll(PDO::FETCH_ASSOC);
    $page = @$results[0];

    // try to get the dataset with rev_language == $lang, to boost the translated content
    if( count($results) == 2 && $page['lang_id'] == Language::getStandardId() ) {
      $page = $results[1];
    }

    return $this->processText($page['id'], $output_type);
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
    global $roscms_subsystem_wiki_path;
    
    $config = &RosCMS::getInstance();

    // try to force unlimited script runtime
    @set_time_limit(0);

    $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, r.id, r.version, r.user_id, r.datetime, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE r.id = :rev_id ORDER BY r.version DESC LIMIT 1");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $page = $stmt->fetch(PDO::FETCH_ASSOC);

    if ($page === false) {
      return; // nothing found
    }

    $this->page_name = $page['name'];
    $this->lang = $page['lang_id'];
    $this->dynamic_num = Tag::getValueByUser($page['id'], 'number', -1);
    $this->output_type = $output_type;
    // replace dynamic tags first and then static

    // get content and replace with other contents
    $content = Data::getText($page['id'], 'content');
    $content = preg_replace_callback('/(\[#templ_[^][#[:space:]]+\])/', array($this,'insertTemplate'), $content);
    $content = preg_replace_callback('/(\[#cont_[^][#[:space:]]+\])/', array($this,'insertContent'), $content);
    $content = preg_replace_callback('/(\[#inc_[^][#[:space:]]+\])/', array($this,'insertScript'), $content);
    $content = preg_replace_callback('/(\[#link_[^][#[:space:]]+\])/', array($this,'insertHyperlink'), $content);

    // website url
    $content = str_replace('[#roscms_path_homepage]', $config->pathGenerated(), $content);

    // replace roscms constants dependent from dynamic number
    if ($this->dynamic_num > 0) {
      $content = str_replace('[#roscms_filename]', $this->page_name.'_'.$this->dynamic_num.'.html', $content);
      $content = str_replace('[#roscms_pagename]', $this->page_name.'_'.$this->dynamic_num, $content); 
      $content = str_replace('[#roscms_pagetitle]', Data::getSText($page['id'], 'title').' #'.$this->dynamic_num, $content); 
    }
    // take care of dynamic number independent entries
    else {
      $content = str_replace('[#roscms_filename]', $this->page_name.'.html', $content); 
      $content = str_replace('[#roscms_pagename]', $this->page_name, $content); 
      $content = str_replace('[#roscms_pagetitle]', Data::getSText($page['id'], 'title'), $content); 
    }

    // replace current language stuff
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, name_short FROM ".ROSCMST_LANGUAGES." WHERE id = :lang LIMIT 1");
    $stmt->bindParam('lang',$page['lang_id'],PDO::PARAM_INT);
    $stmt->execute();
    $lang=$stmt->fetchOnce(PDO::FETCH_ASSOC);
    $content = str_replace('[#roscms_language]', $lang['name'], $content); 
    $content = str_replace('[#roscms_language_short]', $lang['name_short'], $content); 

    // @REMOVEME if it is no more present in Database
    $content = str_replace('[#roscms_format]', 'html', $content); 

    // replace date and time
    $content = str_replace('[#roscms_date]', date('Y-m-d'), $content); 
    $localtime = localtime(time() , 1);
    $content = str_replace('[#roscms_time]', sprintf('%02d:%02d', $localtime['tm_hour'],$localtime['tm_min']), $content);

    // replace with user_name
    // @FIXME broken logic, or one link too much, which should be removed from Database
    $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_USERS." WHERE id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',ThisUser::getInstance()->id());
    $stmt->execute();
    $user_name = $stmt->fetchColumn();
    $content = str_replace('[#roscms_user]', $user_name, $content); // account that generate
    $content = str_replace('[#roscms_inc_author]', $user_name, $content); // account that changed the include text

    // page version
    $content = str_replace('[#roscms_page_version]', $page['version'], $content); 

    // page edit link
    $content = str_replace('[#roscms_page_edit]', $config->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$this->page_name.'&amp;d_val2='.$this->lang.'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit', $content);

    // Subsystem Links
    $content = preg_replace('/\[#wiki_([^]]+)\]/', $roscms_subsystem_wiki_path.'$1', $content);

    // translation info
    if ($page['lang_id'] == Language::getStandardId()) {
      $content = str_replace('[#roscms_trans]', '<p><a href="'.$config->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$this->page_name.'&amp;d_val2='.$page['lang_id'].'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit" style="font-size:10px !important;">Edit page content</a> (RosCMS translator account membership required, visit the <a href="'.$config->pathGenerated().'forum/" style="font-size:10px !important;">website forum</a> for help)</p><br />', $content); 
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_LANGUAGES." WHERE id=:lang_id");
      $stmt->bindParam('lang_id',Language::getStandardId(),PDO::PARAM_INT);
      $stmt->execute(),
      $lang=$stmt->fetchColumn();
    
      $content = str_replace('[#roscms_trans]', '<p><em>If the translation of the <a href="'.$config->pathGenerated().'?page='.$page['name'].'&amp;lang='.Language::getStandardId().'" style="font-size:10px !important;">'.$lang['name'].' language</a> of this page appears to be outdated or incorrect, please check-out the <a href="'.$config->pathGenerated().'?page='.$page['name'].'&amp;lang='.Language::getStandardId().'" style="font-size:10px !important;">'.$lang['name'].'</a> page and <a href="'.$config->pathGenerated().'forum/viewforum.php?f=18" style="font-size:10px !important;">report</a> or <a href="'.$config->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$page['name'].'&amp;d_val2='.$page['lang_id'].'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit" style="font-size:10px !important;">update the content</a>.</em></p><br />', $content); 
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
    $mode = $_GET['d_val4'];
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
        $page_link = RosCMS::getInstance()->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=index&amp;d_val2='.$this->lang.'&amp;d_val3=';
      }
      else {
        $page_link = RosCMS::getInstance()->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$page_name_with_num.'&amp;d_val2='.$this->lang.'&amp;d_val3='.$dynamic_num;
      }

      if (isset($_GET['d_val4']) && $_GET['d_val4'] == 'edit') {
        $page_link .= '&amp;d_val4=edit';
      }
    }

    // static pages
    else { 
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE d.name  = :name AND r.lang_id = :lang AND version > 0 LIMIT 1");
      $stmt->bindParam('name',$page_name,PDO::PARAM_STR);
      $stmt->bindParam('lang',$this->lang,PDO::PARAM_STR);
      $stmt->execute();

      // get page extension
      $link_extension = Tag::getValueByUser($stmt->fetchColumn(), 'extension', -1);
      if ($link_extension == '') {
        $link_extension = 'html';
      }

      // get page link
      if ($page_name == '') {
        $page_link = RosCMS::getInstance()->pathGenerated().$this->lang.'/404.html';
      }
      else{
        $page_link = RosCMS::getInstance()->pathGenerated().$this->lang.'/'.$page_name.'.'.$link_extension;
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
    // get entry
    $stmt=&DBConnection::getInstance()->prepare("SELECT a.name AS access_name, t.content, r.version, r.user_id, r.datetime , r.data_id, r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_TEXT." t ON t.rev_id = r.id JOIN ".ROSCMST_ACCESS." a ON a.id=d.acl_id WHERE d.name = :name AND d.type = :type AND r.version > 0 AND r.lang_id IN(:lang_one, :lang_two) AND t.name = 'content' ORDER BY r.version DESC LIMIT 2");
    $stmt->bindParam('name',$match_name,PDO::PARAM_STR);
    $stmt->bindParam('type',$match_type,PDO::PARAM_STR);
    $stmt->bindParam('lang_one',$lang,PDO::PARAM_INT);
    $stmt->bindParam('lang_two',Language::getStandardId(),PDO::PARAM_INT);
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
    if (count($results) == 2 && $content['lang_id'] == Language::getStandardId() ) {
      $content = $results[1];
    }

    // get basicly content
    $html_content = $content['content'];

    // execute script and get echoed content
    if ($match_type == 'script' && Tag::getValue($content['id'], 'kind') == 'php') {
      $html_content = $this->evalTemplate($html_content, $this->dynamic_num, $lang);
    }

    // text for username
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, fullname FROM ".ROSCMST_USERS." WHERE id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',$content['user_id'],PDO::PARAM_INT);
    $stmt->execute();
    $account = $stmt->fetchOnce(PDO::FETCH_ASSOC);
    if ($account['fullname'] != '') {
      $user_text = $account['fullname'].' ('.$account['name'].')';
    }
    else {
      $user_text = $account['name'];
    }

    // last modified ... by ...
    $html_content = str_replace('[#roscms_'.$match_type.'_version]', '<em>Last modified: '.$content['datetime'].', rev. '.$content['version'].' by '.$user_text.'</em>', $html_content); 

    // preview-edit-mode (add some html through it)
    if (isset($_GET['d_fl']) && $_GET['d_fl'] == 'edit' && $content['access_name'] == 'default' && $match_type != 'script') {
      $html_content = '<div style="border: 1px dashed red;"><div style="padding: 2px;"><a href="'.RosCMS::getInstance()->pathRosCMS().'?page=data&amp;branch=website&amp;edit=rv'.$content['data_id'].'|'.$content['id'].'" style="background-color:#E8E8E8;"> <img src="'.RosCMS::getInstance()->pathRosCMS().'images/edit.gif" style="width:19px; height:19px; border:none;" /><em>'.$match_name.'</em> </a></div>'.$html_content.'</div>';
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
