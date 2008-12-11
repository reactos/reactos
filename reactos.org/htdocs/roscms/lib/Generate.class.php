<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>
                  2008  Danny Götte <dangerground@web.de>

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
    
function getTagValueG($data_id, $rev_id, $user, $name) {
  return Tag::getValue($rev_id, $name, $user);
}

/**
 * class Generate
 * 
 */
class Generate
{

  private $destination_folder = '../'; // distance between roscms folder to pages folder
  private $cache_dir = './../roscms_cache/'; // cached files

  private $outout_type; // 'show' => preview; 'generate' => while generating files

  // page related vars
  private $page_name;
  private $page_version;
  private $dynamic_num = false; // should be a number (false => no dynamic number)
  
  private $short = array('template'=>'templ', 'content'=>'cont', 'script'=>'inc');



  /**
   * 
   * @param output_type 'show' => preview; 'generate' => while generating files
   * @return 
   * @access public
   */
  public function __construct( $output_type = 'generate' )
  {
    $this->output_type = $output_type;
    $this->begin = date('Y-m-d H:i:s');

    //@DEPRACTED
    mysql_connect(DB_HOST, DB_USER, DB_PASS);
    mysql_select_db(DB_NAME);

    // try to force unlimited script runtime
    @set_time_limit(0);
  }



  /**
   * 
   *
   * @return 
   * @access public
   */
  public function allEntries( $id = null, $id_type = 'lang' )
  {
    //
    clearstatcache();

    // caching
    //$this->cacheFiles();

    // build all entries
    if ($id === null) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE d.type = 'page' ORDER BY l.level DESC, l.id ASC, d.name ASC");
    }

    // build only the selected language
    elseif($id_type === 'lang') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE d.type = 'page' AND l.id=:lang_id ORDER BY d.name ASC");
      $stmt->bindParam('lang_id',$id,PDO::PARAM_INT);
    }

    // build only the selected page, in all languages
    elseif($id_type === 'data') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE d.type = 'page' AND d.name = :data_name ORDER BY l.level DESC");
      $stmt->bindParam('data_name',$ld,PDO::PARAM_STR);
    }
    $stmt->execute();

    $old_lang = false;
    while ($data = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // display language
      if ($old_lang != $data['lang_id']) {
        echo '<span style="text-decoration:underline;">'.$data['language'].'</span>';
        $old_lang = $data['lang_id'];

        $this->lang_id = $data['lang_id'];
        $this->lang = $data['lang_short'];

        // create new language folder
        if (isset($data['lang_short']) && !is_dir($this->destination_folder.'/'.$data['lang_short'])) {
          mkdir($this->destination_folder.'/'.$data['lang_short']);
        }
      }

      // generate Entry, prefer native language
      $this->oneEntry($data['name']);
    }
  } // end of member function doAll



  /**
   * 
   *
   * @return 
   * @access public
   */
  public function oneEntry( $data_name, $lang_id = null )
  {
    // if called standalone
    if ($lang_id != null) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT name_short FROM ".ROSCMST_LANGUAGES." WHERE id = :lang_id LIMIT 1");
      $stmt->bindParam('lang_id',$lang_id,PDO::PARAM_INT);
      $stmt->execute();

      $this->lang = $stmt->fetchColumn();
      $this->lang_id = $lang_id;
    }

    // get page data
    $revision = $this->getFrom('page', $data_name);

    if ($revision === false) {
      echo '<p><strong>!!! '.date('Y-m-d H:i:s').' - no text found: '.$data_name.'('.$this->lang_id.')</strong></p>';
      return false;
    }

    // check file extension
    $file_extension = Tag::getValueByUser($revision['id'], 'extension', -1);
    if ($file_extension === false) {
      echo '<p><strong>!!! '.date('Y-m-d H:i:s').' - file extension missing: '.$data_name.'('.$revision['id'].', '.$this->lang_id.')</strong></p>';
      return false;
    }

    // get file name
    $file_name = $data_name.'.'.$file_extension;
    $file = $this->lang.'/'.$file_name;

    // can I copy from standard lang ?
    if ($this->lang_id == Language::getStandardId() && file_exists($this->destination_folder.RosCMS::getInstance()->siteLanguage().'/'.$file_name)) {
      copy($this->destination_folder.RosCMS::getInstance()->siteLanguage().'/'.$file_name,$this->destination_folder.$file);
    }
    else {

      // needed by replacing functions
      $this->page_name = $data_name;
      $this->page_version = $revision['version'];
      $this->rev_id = $revision['id'];

      // file content
      $content = $revision['content'];
      $content = str_replace('[#%NAME%]', $data_name, $content);
      $content = str_replace('[#cont_%NAME%]', '[#cont_'.$data_name.']', $content);

      $content = preg_replace_callback('/\[#((cont|inc|templ)_[^][#[:space:]]+)\]/', array($this,'getCached'),$content);

      // replace roscms vars
      $content = $this->replaceRoscmsPlaceholder($content);

      // write content to filename, if possible
      $fh = fopen($this->destination_folder.$file, 'w');
      if ($fh !== false){
        flock($fh,2);
        fputs($fh,$content.'<!-- Generated with '.RosCMS::getInstance()->systemBrand().' ('.RosCMS::getInstance()->systemVersion().') - '.date('Y-m-d H:i:s').' -->'); 
        flock($fh,3);
        fclose($fh);
        //echo '<p> * '.date('Y-m-d H:i:s').' - "'.$file.'"</p>';
        return true;
      }
      else {
        //echo '<p>* could not create file: "'.$this->destination_folder.$file.'"</p>';
        return false;
      }
    }
  } // end of member function oneEntry



  /**
   * 
   *
   * @return 
   * @access private
   */
  private function cacheFiles( $data_id = null )
  {
    
    $standard_lang = Language::getStandardId();

    if ($data_id === null) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.id AS data_id, d.type, d.name, l.id AS lang_id FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE d.type = 'content' OR d.type = 'script' OR d.type='template' ORDER BY l.level DESC");
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.id AS data_id, d.type, d.name, l.id AS lang_id FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE d.id=:data_id ORDER BY l.level DESC");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    }
    $stmt->execute();

    // prepare for usage in loop
      $stmt_more=&DBConnection::getInstance()->prepare("SELECT d.id, d.type, d.name FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON w.depency_id=d.id WHERE w.rev_id=:rev_id AND w.is_include IS TRUE");
    while ($data = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $filename = $this->short[$data['type']].'_'.$data['name'].'.rcf';
      $file = $data['lang_id'].'/'.$filename;

      $revision = $this->getFrom($data['type'],$data['name']);
      
      // can I copy from standard language ?
      if ($revision['lang_id'] == $standard_lang && file_exists($this->cache_dir.$standard_lang.'/'.$filename)) {
        copy($this->cache_dir.$standard_lang.'/'.$filename, $this->cache_dir.$file);
      }

      // generate file
      else {
        $this->rev_id = $revision['id'];
        $this->lang_id = $data['lang_id'];

        $content = $revision['content'];
        
        if($data['type'] == 'script' && Tag::getValue($revision['id'], 'kind') == 'php') {
          $content = $this->evalTemplate($content);
        }

        // replace links
        $content = preg_replace_callback('/\[#link_([^][#[:space:]]+)\]/', array($this, 'replaceWithHyperlink'), $content);

        // process depencies first
        $stmt_more->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
        $stmt_more->execute();
        $depencies = $stmt_more->fetchAll(PDO::FETCH_ASSOC);
        foreach ($depencies as $depency) {
          $depency_file = $data['lang_id'].'/'.$this->short[$depency['type']].'_'.$depency['name'].'.rcf';
          if (!file_exists($this->cache_dir.$depency_file) || $this->begin > date('Y-m-d H:i:s',filemtime($this->cache_dir.$depency_file))) {
            $this->cacheFiles($depency['id']);
          }

          // replace
          $content = str_replace('[#'.$this->short[$depency['type']].'_'.$depency['name'].']', $this->getCached(array(null, $this->short[$depency['type']].'_'.$depency['name'])), $content);
        }

        if (!file_exists($this->cache_dir.$file) ||$this->begin > date('Y-m-d H:i:s',filemtime($this->cache_dir.$file))) {

          // create folder, if necessary
          if(!is_dir($this->cache_dir.$data['lang_id'].'/')) {
            mkdir($this->cache_dir.$data['lang_id'].'/');
          }

          // schreiben
          $fh = fopen($this->cache_dir.$file, 'w');
          if ($fh !== false) {
            flock($fh, 2);
            fwrite($fh, $content); 
            flock($fh, 3);
            fclose($fh);
          }
        }
      }

    } // end while
  } // end of member function cacheFile



  /**
   * 
   *
   * @return 
   * @access public
   */
  private function replaceRoscmsPlaceholder( $content )
  {
    $config = &RosCMS::getInstance();

    // website url
    $content = str_replace('[#roscms_path_homepage]', $config->pathGenerated(), $content);

    // take care of dynamic number independent entries
    if ($this->dynamic_num === false) {
      $content = str_replace('[#roscms_filename]', $this->page_name.'.html', $content); 
      $content = str_replace('[#roscms_pagename]', $this->page_name, $content); 
      $content = str_replace('[#roscms_pagetitle]', Data::getSText($this->rev_id, 'title'), $content); 
    }

    // replace roscms constants dependent from dynamic number
    else {
      $content = str_replace('[#roscms_filename]', $this->page_name.'_'.$this->dynamic_num.'.html', $content);
      $content = str_replace('[#roscms_pagename]', $this->page_name.'_'.$this->dynamic_num, $content); 
      $content = str_replace('[#roscms_pagetitle]', Data::getSText($this->rev_id, 'title').' #'.$this->dynamic_num, $content); 
    }

    // page version
    $content = str_replace('[#roscms_page_version]', $this->page_version, $content); 

    // replace current language stuff
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, name_short FROM ".ROSCMST_LANGUAGES." WHERE id = :lang LIMIT 1");
    $stmt->bindParam('lang',$this->lang_id,PDO::PARAM_INT);
    $stmt->execute();
    $lang=$stmt->fetchOnce(PDO::FETCH_ASSOC);

    $content = str_replace('[#roscms_language]', $lang['name'], $content); 
    $content = str_replace('[#roscms_language_short]', $lang['name_short'], $content); 

    // replace date and time
    $content = str_replace('[#roscms_date]', date('Y-m-d'), $content); 
    $content = str_replace('[#roscms_time]', date('H:i'), $content);

    // replace with user_name
    $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_USERS." WHERE id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',ThisUser::getInstance()->id());
    $stmt->execute();
    $user_name = $stmt->fetchColumn();
    $content = str_replace('[#roscms_user]', $user_name, $content);

    // page edit link
    $content = str_replace('[#roscms_page_edit]', $config->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$this->page_name.'&amp;d_val2='.$this->lang_id.'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit', $content);

    // translation info
    if ($this->lang_id == Language::getStandardId()) {
      $content = str_replace('[#roscms_trans]', '<p><a href="'.$config->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$this->page_name.'&amp;d_val2='.$this->lang_id.'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit" style="font-size:10px !important;">Edit page content</a> (RosCMS translator account membership required, visit the <a href="'.$config->pathGenerated().'forum/" style="font-size:10px !important;">website forum</a> for help)</p><br />', $content); 
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_LANGUAGES." WHERE id=:lang_id");
      $stmt->bindParam('lang_id',Language::getStandardId(),PDO::PARAM_INT);
      $stmt->execute();
      $lang=$stmt->fetchColumn();
    
      $content = str_replace('[#roscms_trans]', '<p><em>If the translation of the <a href="'.$config->pathGenerated().'?page='.$this->page_name.'&amp;lang='.Language::getStandardId().'" style="font-size:10px !important;">'.$lang['name'].' language</a> of this page appears to be outdated or incorrect, please check-out the <a href="'.$config->pathGenerated().'?page='.$this->page_name.'&amp;lang='.Language::getStandardId().'" style="font-size:10px !important;">'.$lang['name'].'</a> page and <a href="'.$config->pathGenerated().'forum/viewforum.php?f=18" style="font-size:10px !important;">report</a> or <a href="'.$config->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$this->page_name.'&amp;d_val2='.$this->lang_id.'&amp;d_val3='.$this->dynamic_num.'&amp;d_val4=edit" style="font-size:10px !important;">update the content</a>.</em></p><br />', $content); 
    }

    // @DEPRACTED if it is no more present in Database
    $content = str_replace('[#roscms_format]', 'html', $content);

    // @DEPRACTED broken logic, or one link too much, which should be removed from Database
    $content = str_replace('[#roscms_inc_author]', $user_name, $content); // account that changed the include text

    return $content;

  
  } // end of member function oneEntry



  /**
   * replace [#link_ABC] with roscms link to ABC
   *
   * @param string[] matches

   * @return 
   * @access private
   */
  private function replaceWithHyperlink( $matches )
  {
    $raw_page_name = $matches[1];
    $dynamic_num = null; // number in page name

    // is dynamic page ?
    if ( preg_match('/^(.+)_([1-9][0-9]*)$/', $raw_page_name, $matches) ) {
      $page_name = $matches[1];
      $dynamic_num = $matches[2];
    }

    // preview
    if ($this->output_type == 'show') {

      //
      if ($page_name == '') {
        $page_link = RosCMS::getInstance()->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val=index&amp;d_val2='.$this->lang_id.'&amp;d_val3=';
      }
      else {
        $page_link = RosCMS::getInstance()->pathRosCMS().'?page=data_out&amp;d_f=page&amp;d_u=show&amp;d_val='.$page_name.'&amp;d_val2='.$this->lang_id.'&amp;d_val3='.$dynamic_num;
      }

      if (isset($_GET['d_val4']) && $_GET['d_val4'] == 'edit') {
        $page_link .= '&amp;d_val4=edit';
      }
    }

    // static pages
    else { 
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE d.name = :name AND d.type = 'page' AND r.version > 0 LIMIT 1");
      $stmt->bindParam('name',$page_name,PDO::PARAM_STR);
      $stmt->execute();

      // get page extension
      $link_extension = Tag::getValueByUser($stmt->fetchColumn(), 'extension', -1);
      if ($link_extension === false) {
        $link_extension = 'html';
      }

      // get page link
      $page_link = $raw_page_name.'.'.$link_extension;
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
  private function getFrom( $type, $name )
  {
    // get entry
    $stmt=&DBConnection::getInstance()->prepare("SELECT t.content, r.id, r.lang_id, r.version FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_TEXT." t ON t.rev_id = r.id WHERE d.name = :name AND d.type = :type AND r.version > 0 AND r.lang_id IN(:lang_one, :lang_two) AND r.archive IS FALSE AND t.name = 'content' ORDER BY r.version DESC LIMIT 2");
    $stmt->bindParam('name',$name,PDO::PARAM_STR);
    $stmt->bindParam('type',$type,PDO::PARAM_STR);
    $stmt->bindParam('lang_one',$this->lang_id,PDO::PARAM_INT);
    $stmt->bindParam('lang_two',Language::getStandardId(),PDO::PARAM_INT);
    $stmt->execute();
    $results=$stmt->fetchAll(PDO::FETCH_ASSOC);

    // check if depency not available
    if (count($results) === 0){
      //echo '<p>* <strong>Not found ('.$type.': '.$name.')</strong></p>';
      return false;
    }

    // try to get the dataset with rev_language == $lang, to boost the translated content
    $revision = $results[0];
    if (count($results) === 2 && $revision['lang_id'] == Language::getStandardId() ) {
      $revision = $results[1];
    }
    return $revision;
  }



  /**
   * 
   *
   * @param string[] matches 

   * @return 
   * @access private
   */
  private function getCached( $matches )
  {
    // get cached content
    $fh = fopen($this->cache_dir.$this->lang_id.'/'.$matches[1].'.rcf', 'r');
    if ($fh !== false){
      $content = fread($fh, filesize($this->cache_dir.$this->lang_id.'/'.$matches[1].'.rcf')+1); 
      fclose($fh);
    }
    else {
      $content = '';
    }

    return $content;
  }



  /**
   * executes code from a script
   *
   * @param string code
   * @return 
   * @access private
   */
  private function evalTemplate( $code )
  {
    // catch output
    ob_start(); 

    // some vars for template usage;
    //@RENAMEME or better remove me, parmeter names should be enough
    $roscms_template_var_pageid = $this->dynamic_num;
    $roscms_template_var_lang = $this->lang_id;

    // execute code and return the output
    eval(' ?> '.$code.' <?php ');
    $output = ob_get_contents(); 
    ob_end_clean(); 
    return $output; 
  } // end of member function eval_template



} // end of AJAX
?>
