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
  private $cache_dir = '../roscms_cache/'; // cached files
  private $base_dir = '';

  private $outout_type; // 'show' => preview; 'generate' => while generating files

  // page related vars
  private $page_name;
  private $page_version;
  private $dynamic_num = false; // should be a number (false => no dynamic number)

  // language
  private $lang_id = null;
  private $lang = null;

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
    @set_time_limit(300);
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
    $this->cacheFiles();

    $this->base_dir = $this->destination_folder;

    // build all entries
    if ($id === null) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, type, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE (d.type = 'page' OR d.type = 'dynamic') ORDER BY l.level DESC, l.id ASC, d.name ASC");
    }

    // build only the selected language
    elseif ($id_type === 'lang') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, type, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE (d.type = 'page' OR d.type = 'dynamic') AND l.id=:lang_id ORDER BY d.name ASC");
      $stmt->bindParam('lang_id',$id,PDO::PARAM_INT);
    }

    // build only the selected page, in all languages
    elseif ($id_type === 'data') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, type, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE (d.type = 'page' OR d.type = 'dynamic') AND d.name = :data_name ORDER BY l.level DESC");
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
      }

      // generate entry
      if ($data['type'] == 'page') {
        $this->oneEntry($data['name']);
      }
      else {
        $this->makeDynamic($data['name']);
      }
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
    if ($lang_id !== null) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT name_short FROM ".ROSCMST_LANGUAGES." WHERE id = :lang_id LIMIT 1");
      $stmt->bindParam('lang_id',$lang_id,PDO::PARAM_INT);
      $stmt->execute();

      $this->lang = $stmt->fetchColumn();
      $this->lang_id = $lang_id;
    }

    echo $this->lang.'--'.$data_name.'<br />';

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

    // can I copy from standard lang ?
    if (!$this->cloneFile($revision['lang_id'],RosCMS::getInstance()->siteLanguage(),$this->lang, $file_name)) {

      // needed by replacing functions
      $this->page_name = $data_name;
      $this->page_version = $revision['version'];
      $this->rev_id = $revision['id'];

      // file content
      $content = $revision['content'];
      $content = str_replace('[#%NAME%]', $data_name, $content);
      $content = str_replace('[#cont_%NAME%]', '[#cont_'.$data_name.']', $content);

      // replace depencies
      $stmt_more=&DBConnection::getInstance()->prepare("SELECT d.id, d.type, d.name FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON w.child_id=d.id WHERE w.rev_id=:rev_id AND w.include IS TRUE");
      $stmt_more->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
      $stmt_more->execute();
      $depencies = $stmt_more->fetchAll(PDO::FETCH_ASSOC);
      foreach ($depencies as $depency) {

        // replace
        if ($depency['type'] != 'script') {
          $content = str_replace('[#'.$this->short[$depency['type']].'_'.$depency['name'].']', $this->getCached(array(null, $this->short[$depency['type']].'_'.$depency['name'])), $content);
        }
        // eval
        else {echo '[#inc_'.$depency['name'].']';
          $content = str_replace('[#inc_'.$depency['name'].']', $this->evalTemplate(array(null,$depency['name'])), $content);
        }
      }

      // replace roscms vars
      $content = $this->replaceRoscmsPlaceholder($content);

      // write content to filename, if possible
      $this->writeFile($this->lang,$file_name, $content.'<!-- Generated with '.RosCMS::getInstance()->systemBrand().' ('.RosCMS::getInstance()->systemVersion().') - '.date('Y-m-d H:i:s').' -->');
    }
  } // end of member function oneEntry



  /**
   * 
   *
   * @return 
   * @access public
   */
  public function makeDynamic( $data_name, $lang_id = null )
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
    $revision = $this->getFrom('dynamic', $data_name);

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

    //
    $next_index = intval(Tag::getValue($revision['id'],'next_index',-1));

    for ($i=1; $i < $next_index; $i++) {

      // get file name
      $file_name = $data_name.'_'.$i.'.'.$file_extension;
      $file = $this->lang.'/'.$file_name;

      // can I copy from standard lang ?
      if (!$this->cloneFile($revision['lang_id'], RosCMS::getInstance()->siteLanguage(), $this->lang, $file_name)) {

        // needed by replacing functions
        $this->page_name = $data_name;
        $this->page_version = $revision['version'];
        $this->rev_id = $revision['id'];
        $this->dynamic_num = $i;

        // file content
        $content = $revision['content'];
        $content = str_replace('[#%NAME%]', $data_name, $content); 

        $content = preg_replace_callback('/\[#((cont|templ)_[^][#[:space:]]+)\]/', array($this,'getCached'),$content);
        $content = preg_replace_callback('/\[#inc_([^][#[:space:]]+)\]/', array($this,'evalTemplate'),$content);

        // replace roscms vars
        $content = $this->replaceRoscmsPlaceholder($content);

        // write content to filename, if possible
        $this->writeFile($this->lang,$file_name, $content.'<!-- Generated with '.RosCMS::getInstance()->systemBrand().' ('.RosCMS::getInstance()->systemVersion().') - '.date('Y-m-d H:i:s').' -->');
      }
    }
  } // end of member function oneEntry



  /**
   * 
   *
   * @return 
   * @access private
   */
  public function update( $rev_id )
  {

    $stmt=&DBConnection::getInstance()->prepare("SELECT data_id, lang_id FROM ".ROSCMST_REVISIONS." WHERE id=:rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision=$stmt->fetchOnce(PDO::FETCH_ASSOC);

    $this->lang_id = $revision['lang_id'];

    // cache data
    $this->cacheFiles($revision['data_id']);

    // set generating dir again
    $this->base_dir = $this->destination_folder;

    // update entries which depends on this one
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.lang_id, d.name, d.type, r.id FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_REVISIONS." r ON r.id=w.rev_id JOIN ".ROSCMST_ENTRIES." d ON d.id=r.data_id WHERE w.child_id=:depency_id AND w.rev_id != :rev_id AND r.archive IS FALSE AND w.include IS TRUE");
    $stmt->bindParam('depency_id',$revision['data_id'],PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while ($depency = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // in standard language we may have depencies to other languages, so better generate them all
      if ($revision['lang_id'] == Language::getStandardId()){
        $stmt_lang=&DBConnection::getInstance()->prepare("SELECT id, name_short FROM ".ROSCMST_LANGUAGES." ORDER BY level DESC");
      }
      else {
        $stmt_lang=&DBConnection::getInstance()->prepare("SELECT id, name_short FROM ".ROSCMST_LANGUAGES." WHERE id=:lang_id");
        $stmt_lang->bindParam('lang_id',$revision['lang_id'],PDO::PARAM_INT);
      }
      $stmt_lang->execute();
      while ($language = $stmt_lang->fetch(PDO::FETCH_ASSOC)) {

      // language settings for generating process
      $this->lang_id=$language['id'];
      $this->lang=$language['name_short'];

        // cache recursivly or generate page
        switch ($depency['type']) {
          case 'page':
            $this->oneEntry($depency['name'], $language['id']);
            break;
          case 'dynamic':
            $this->makeDynamic($depency['name'], $language['id']);
            break;
          case 'script':
            // scripts are only executed by in pages
            break;
          default:
            $this->update($depency['id']);
            break;
        }
      }
    }
  }



  /**
   * 
   *
   * @return 
   * @access private
   */
  private function cacheFiles( $data_id = null, $depencies = true )
  {
    $this->base_dir = $this->cache_dir;

    if ($data_id === null) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.id AS data_id, d.type, d.name, l.id AS lang_id FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE d.type = 'content' OR d.type = 'script' OR d.type='template' ORDER BY l.level DESC");
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.id AS data_id, d.type, d.name, l.id AS lang_id FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE d.id=:data_id ORDER BY l.level DESC");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    }
    $stmt->execute();

    // prepare for usage in loop
      $stmt_more=&DBConnection::getInstance()->prepare("SELECT d.id, d.type, d.name FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON w.child_id=d.id WHERE w.rev_id=:rev_id AND w.include IS TRUE AND d.type != 'script'");

    while ($data = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // change language only on top level
      if($data_id === null){
        $this->lang_id = $data['lang_id'];
      }

      $filename = $this->short[$data['type']].'_'.$data['name'].'.rcf';
      $file = $data['lang_id'].'/'.$filename;

      $revision = $this->getFrom($data['type'],$data['name']);
      
      // can I copy from standard language ?
      if (!$this->cloneFile($revision['lang_id'],Language::getStandardId(),$data['lang_id'], $filename)) {

        // generate file
        $this->rev_id = $revision['id'];
        if (preg_match('/_([1-9][0-9]*)$/', $data['name'],$matches)) {
          $this->dynamic_num = $matches[1];
        }
        else {
          $this->dynamic_num = false;
        }
        $content = $revision['content'];

        // replace links
        $content = preg_replace_callback('/\[#link_([^][#[:space:]]+)\]/', array($this, 'replaceWithHyperlink'), $content);

        // process depencies first
        $stmt_more->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
        $stmt_more->execute();
        $depencies = $stmt_more->fetchAll(PDO::FETCH_ASSOC);
        foreach ($depencies as $depency) {

          // do we care about depencies ?
          if ($depencies) {
            $depency_file = $data['lang_id'].'/'.$this->short[$depency['type']].'_'.$depency['name'].'.rcf';
            if (!file_exists($this->cache_dir.$depency_file) || $this->begin > date('Y-m-d H:i:s',filemtime($this->cache_dir.$depency_file))) {
              $this->cacheFiles($depency['id']);
            }
          }

          // replace
          $content = str_replace('[#'.$this->short[$depency['type']].'_'.$depency['name'].']', $this->getCached(array(null, $this->short[$depency['type']].'_'.$depency['name'])), $content);
        }

        $this->writeFile($data['lang_id'],$filename, $content);
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
    $content = str_replace('[#roscms_language_id]', $this->lang_id, $content); 

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
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE d.name = :name AND (d.type='page' OR d.type='dynamic') AND r.version > 0 LIMIT 1");
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
    $stmt=&DBConnection::getInstance()->prepare("SELECT t.content, r.id, r.lang_id, r.version FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_TEXT." t ON t.rev_id = r.id JOIN ".ROSCMST_TAGS." ta ON ta.rev_id=r.id WHERE d.name = :name AND d.type = :type AND r.version > 0 AND r.lang_id IN(:lang_one, :lang_two) AND r.archive IS FALSE AND t.name = 'content' AND ta.name='status' AND ta.value='stable' ORDER BY r.version DESC LIMIT 2");
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
  private function writeFile( $dir, $filename, $content )
  {
    $dir .= '/';
    $file = $dir.$filename;

    if (!file_exists($this->base_dir.$file) || $this->begin > date('Y-m-d H:i:s',filemtime($this->base_dir.$file))) {

      // create folder, if necessary
      if(!is_dir($this->base_dir.$dir)) {
        mkdir($this->base_dir.$dir);
      }

      // schreiben
      $fh = fopen($this->base_dir.$file, 'w');
      if ($fh !== false) {
        flock($fh, 2);
        fwrite($fh, $content); 
        flock($fh, 3);
        fclose($fh);
        return true;
      }
    }
    return false;
  } // end of member function writeFile



  /**
   * 
   *
   * @param string[] matches 

   * @return 
   * @access private
   */
  private function cloneFile( $source_lang, $source_folder, $dest_folder, $filename )
  {
    $standard_lang = &Language::getStandardId();

    if ($source_lang == $standard_lang && $source_lang != $standard_lang && $this->lang_id != $standard_id && file_exists($this->base_dir.$source_folder.'/'.$filename)) {
      return copy($this->base_dir.$source_folder.'/'.$filename, $this->base_dir.$dest_folder.'/'.$filename);
    }
    return false;

  } // end of member function tryCopy



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
  private function evalTemplate( $matches )
  {
    $revision = $this->getFrom('script',$matches[1]);
  
    if( Tag::getValue($revision['id'], 'kind',-1) == 'php') {

      // catch output
      ob_start();

      // some vars for template usage;
      $roscms_dynamic_number = $this->dynamic_num;
      $roscms_lang_id = $this->lang_id;

      // execute code and return the output
      eval($revision['content']);
      $content = ob_get_contents(); 
      ob_end_clean(); 
    }
    else {

      $content = $revision['content'];
    }

    // replace roscms links
    $content = preg_replace_callback('/\[#link_([^][#[:space:]]+)\]/', array($this, 'replaceWithHyperlink'), $content);

    return $content;
  } // end of member function eval_template



} // end of AJAX
?>
