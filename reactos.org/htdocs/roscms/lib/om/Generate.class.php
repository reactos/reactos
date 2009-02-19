<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007      Klemens Friedl <frik85@reactos.org>
                  2008-2009 Danny Götte <dangerground@web.de>

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
 * class Generate
 * 
 */
class Generate
{

  private $cache_dir; // cached files
  private $base_dir; // where current things are generated into (may switch between normal generation and caching generation)

  // page related vars
  private $page_name;
  private $page_version;
  private $dynamic_num = false; // should be a number (false => no dynamic number)

  // language
  private $lang_id = null;
  private $lang = null;

  private $short = array('content'=>'cont', 'script'=>'inc');



  /**
   * 
   * @return 
   * @access public
   */
  public function __construct( )
  {
    // setup paths for generating content and caching
    $this->base_dir = ROSCMS_PATH.RosCMS::getInstance()->pathGenerated();
    $this->cache_dir = ROSCMS_PATH.RosCMS::getInstance()->pathGenerationCache();

    // set generation start time
    $this->begin = date('Y-m-d H:i:s');

    // try to force a bigger script runtime (needed by some functions)
    @set_time_limit(0);
  } // end of constructor



  /**
   * gives a preview that revision, as it will be displayed later
   * there may exist some problems with stylesheets, but thats no bug
   *
   * @param int rev_id
   * @return string
   * @access public
   */
  public function preview( $rev_id )
  {
    // register rev_id for class wide usage
    $this->rev_id=$rev_id;

    // get language of that revision, and register it
    $stmt=&DBConnection::getInstance()->prepare("SELECT lang_id FROM ".ROSCMST_REVISIONS." WHERE id=:rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $this->lang_id = $stmt->fetchColumn();

    // get revision content text
    $stmt=&DBConnection::getInstance()->prepare("SELECT content FROM ".ROSCMST_TEXT." WHERE rev_id=:rev_id AND name='content'");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $content = $stmt->fetchColumn();

    // replace depencies
    $stmt=&DBConnection::getInstance()->prepare("SELECT d.type, d.name FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON w.child_id=d.id WHERE w.rev_id=:rev_id AND w.include IS TRUE");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    while ($depency = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // replace
      if ($depency['type'] != 'script') {
        $content = str_replace('[#'.$this->short[$depency['type']].'_'.$depency['name'].']', $this->getCached(array(null, $this->short[$depency['type']].'_'.$depency['name'])), $content);
      }
    }

    // execute embedded scripts
    $content = preg_replace_callback('/\[#inc_([^][#[:space:]]+)\]/', array($this,'evalScript'),$content);

    // replace roscms vars
    $content = $this->replaceRoscmsPlaceholder($content);

    // replace links
    $content = preg_replace_callback('/\[#link_([^][#[:space:]]+)\]/', array($this, 'replaceWithHyperlink'), $content);

    echo $content;
  } // end of member function preview



  /**
   * generate files for all revisions
   *
   * @param id
   * @param id_type 'lang', 'data', 'revision'
   * @access public
   */
  public function allEntries( $id = null, $id_type = 'lang' )
  {
    // clear filesystem cache from host
    clearstatcache();

    // caching
    if ($id_type != 'revision') {
      $this->cacheFiles();
    }

    // build all entries
    if ($id === null) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, type, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE (d.type = 'page' OR d.type = 'dynamic') ORDER BY l.level DESC, l.name ASC, d.name ASC");
    }

    // build only the selected language
    elseif ($id_type === 'lang') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, type, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE (d.type = 'page' OR d.type = 'dynamic') AND l.id=:lang_id ORDER BY d.name ASC");
      $stmt->bindParam('lang_id',$id,PDO::PARAM_INT);
    }

    // build only the selected page, in all languages
    elseif ($id_type === 'data') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, type, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE (d.type = 'page' OR d.type = 'dynamic') AND d.name = :data_name ORDER BY l.level DESC, l.name ASC");
      $stmt->bindParam('data_name',$ld,PDO::PARAM_STR);
    }

    // build only the revision
    elseif ($id_type === 'revision') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.name, type, l.id AS lang_id, l.name AS language, l.name_short AS lang_short FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id=d.id JOIN ".ROSCMST_LANGUAGES." l ON l.id=r.lang_id WHERE (d.type = 'page' OR d.type = 'dynamic') AND r.id = :rev_id");
      $stmt->bindParam('rev_id',$ld,PDO::PARAM_STR);
    }
    $stmt->execute();

    $old_lang = false;
    while ($data = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // display language
      if ($old_lang != $data['lang_id']) {
        echo '<span style="text-decoration:underline;font-size:1.3em;">'.$data['language'].'</span><br />';
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
    } // end while
  } // end of member function allEntries



  /**
   * This function writes one revision as file.
   *
   * @param string data_name
   * @return bool
   * @access private
   */
  private function oneEntry( $data_name )
  {
    $this->dynamic_num = false;

    // get page data
    $revision = $this->getFrom('page', $data_name);
    if ($revision === false) {
      echo '<p><strong>!!! '.date('Y-m-d H:i:s').' - no text found: '.$data_name.'('.$this->lang_id.')</strong></p>';
      return false;
    }

    // check file extension
    $file_extension = Tag::getValue($revision['id'], 'extension', -1);
    if ($file_extension === false) {
      echo '<p><strong>!!! '.date('Y-m-d H:i:s').' - file extension missing: '.$data_name.'('.$revision['id'].', '.$this->lang_id.')</strong></p>';
      return false;
    }

    // construct file name
    $file_name = $data_name.'.'.$file_extension;

    // information, what was generated
    echo $file_name.'<br />';

    // needed by replacing functions
    $this->page_name = $data_name;
    $this->rev_id = $revision['id'];

    // file content
    $content = $revision['content'];

    // replace depencies
    $stmt_more=&DBConnection::getInstance()->prepare("SELECT d.id, d.type, d.name FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON w.child_id=d.id WHERE w.rev_id=:rev_id AND w.include IS TRUE");
    $stmt_more->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
    $stmt_more->execute();
    while ($depency = $stmt_more->fetch(PDO::FETCH_ASSOC)) {

      // replace
      if ($depency['type'] != 'script') {
        $content = str_replace('[#'.$this->short[$depency['type']].'_'.$depency['name'].']', $this->getCached(array(null, $this->short[$depency['type']].'_'.$depency['name'])), $content);
      }
    } // end foreach

    // execute scripts
    $content = preg_replace_callback('/\[#inc_([^][#[:space:]]+)\]/', array($this,'evalScript'),$content);

    // replace roscms vars
    $content = $this->replaceRoscmsPlaceholder($content);

    // write content to filename, if possible
    return $this->writeFile($this->lang,$file_name, $content.'<!-- Generated with '.RosCMS::getInstance()->systemBrand().' ('.RosCMS::getInstance()->systemVersion().') - '.date('Y-m-d H:i:s').' -->');
  } // end of member function oneEntry



  /**
   * generate all sub-pages of a dynamic page
   *
   * @return bool
   * @access private
   */
  private function makeDynamic( $data_name, $number = null )
  {
    // get page data
    $revision = $this->getFrom('dynamic', $data_name);

    if ($revision === false) {
      echo '<p><strong>!!! '.date('Y-m-d H:i:s').' - no text found: '.$data_name.'('.$this->lang_id.')</strong></p>';
      return false;
    }

    // check file extension
    $file_extension = Tag::getValue($revision['id'], 'extension', -1);
    if ($file_extension === false) {
      echo '<p><strong>!!! '.date('Y-m-d H:i:s').' - file extension missing: '.$data_name.'('.$revision['id'].', '.$this->lang_id.')</strong></p>';
      return false;
    }

    // generate all numbers
    if ($number === null) {
      // get last index
      $next_index = (int)Tag::getValue($revision['id'],'next_index',-1);
      $start = 1;
    }

    // generate only one number
    else {
      $start = $number;
      $next_index = $start + 1;
    }

    for ($i=$start; $i < $next_index; ++$i) {

      // get file name
      $file_name = $data_name.'_'.$i.'.'.$file_extension;

      // information, what was generated
      echo $this->lang_id.'--'.$file_name.'<br />';
      
      // get current instance (dynamic entry revision id)
      $stmt=&DBConnection::getInstance()->prepare("SELECT r.id, r.lang_id FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON d.id=r.data_id WHERE d.type='content' AND d.name = CONCAT(:parent_name,'_',:dynamic_num) AND r.status='stable' AND r.archive IS FALSE LIMIT 1");
      $stmt->bindParam('parent_name',$revision['name'],PDO::PARAM_STR);
      $stmt->bindParam('dynamic_num',$i,PDO::PARAM_INT);
      $stmt->execute();
      $instance = $stmt->fetchOnce(PDO::FETCH_ASSOC);

      // needed by replacing functions
      $this->page_name = $data_name;
      $this->rev_id = $revision['id'];
      $this->dynamic_num = $i;

      // replace depencies
      $stmt_more=&DBConnection::getInstance()->prepare("SELECT d.id, d.type, d.name FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON w.child_id=d.id WHERE w.rev_id=:rev_id AND w.include IS TRUE");
      $stmt_more->bindParam('rev_id',$instance['id'],PDO::PARAM_INT);
      $stmt_more->execute();
      while ($depency = $stmt_more->fetch(PDO::FETCH_ASSOC)) {

        // replace
        if ($depency['type'] != 'script') {
          $content = str_replace('[#'.$this->short[$depency['type']].'_'.$depency['name'].']', $this->getCached(array(null, $this->short[$depency['type']].'_'.$depency['name'])), $content);
        }
      } // end foreach

      // replace scripts
      $content = preg_replace_callback('/\[#inc_([^][#[:space:]]+)\]/', array($this,'evalScript'),$content);

      // replace roscms vars
      $content = $this->replaceRoscmsPlaceholder($content);

      // write content to filename, if possible
      $this->writeFile($this->lang,$file_name, $content.'<!-- Generated with '.RosCMS::getInstance()->systemBrand().' ('.RosCMS::getInstance()->systemVersion().') - '.date('Y-m-d H:i:s').' -->');
    } // end for
  } // end of member function makeDynamic



  /**
   * updates all entries, which rely on the given rev_id
   *
   * @param int rev_id
   * @param bool
   * @access private
   */
  public function update( $rev_id )
  {
    static $base_rev;

    // exclude the base ref to avoid circles
    if (empty($this->base_rev)) {
      $base_rev = $rev_id;
    }

    // get revision information
    $stmt=&DBConnection::getInstance()->prepare("SELECT data_id, lang_id FROM ".ROSCMST_REVISIONS." WHERE id=:rev_id");
    $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision=$stmt->fetchOnce(PDO::FETCH_ASSOC);

    // cache revision (set language, cache)
    $this->lang_id = $revision['lang_id'];
    $this->cacheFiles($revision['data_id']);
    
    // for usage in loop
      // in standard language we may have depencies to other languages, so better generate them all
      if ($revision['lang_id'] == Language::getStandardId()){
        $stmt_lang=&DBConnection::getInstance()->prepare("SELECT id, name_short FROM ".ROSCMST_LANGUAGES." ORDER BY level DESC, name ASC");
      }
      else {
        $stmt_lang=&DBConnection::getInstance()->prepare("SELECT id, name_short FROM ".ROSCMST_LANGUAGES." WHERE id=:lang_id");
        $stmt_lang->bindParam('lang_id',$revision['lang_id'],PDO::PARAM_INT);
      }

    // get list of entries which depend on this one and handle their types
    $stmt=&DBConnection::getInstance()->prepare("
        SELECT
          org.name, org.type, COALESCE( trans.id, org.id ) AS id, org.data_id
        FROM (
          SELECT d.name, d.type, r.id, r.data_id FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_REVISIONS." r ON r.id=w.rev_id JOIN ".ROSCMST_ENTRIES." d ON d.id=r.data_id WHERE w.child_id=:depency_id AND r.lang_id = :standard_lang AND w.rev_id NOT IN(:rev_id,:rev_id2) AND r.archive IS FALSE AND w.include IS TRUE
        ) AS org LEFT OUTER JOIN (
          SELECT d.name, d.type, r.id, r.data_id FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_REVISIONS." r ON r.id=w.rev_id JOIN ".ROSCMST_ENTRIES." d ON d.id=r.data_id WHERE w.child_id=:depency_id AND r.lang_id = :lang_id AND w.rev_id NOT IN(:rev_id,:rev_id2) AND r.archive IS FALSE AND w.include IS TRUE
        ) AS trans ON org.data_id = trans.data_id");
    $stmt->bindParam('depency_id',$revision['data_id'],PDO::PARAM_INT);
    $stmt->bindParam('rev_id',$base_rev,PDO::PARAM_INT);
    $stmt->bindParam('rev_id2',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('standard_lang',Language::getStandardId(),PDO::PARAM_INT);
    $stmt->bindParam('lang_id',$revision['lang_id'],PDO::PARAM_INT);
    $stmt->execute();
    while ($depency = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // cache recursivly or generate page
      switch ($depency['type']) {
        case 'page':
        case 'dynamic':

          // generate pages for all languages, if standard lang, otherwise only once
          $stmt_lang->execute();
          while ($language = $stmt_lang->fetch(PDO::FETCH_ASSOC)) {

            // language settings for generating process
            $this->lang_id=$language['id'];
            $this->lang=$language['name_short'];

            // seperate functions for pages & dynamic pages (in that order)
            if($depency['type'] == 'page') {
              $this->oneEntry($depency['name'], $language['id']);
            }
            else {
              $this->makeDynamic($depency['name'], $language['id']);
            }
          } // end while language
          break;

        case 'script':
          // scripts are only executed in pages
          break;
        default:

          // only run update once per $rev_id
          $this->update($depency['id']);
          break;
      } // end switch
    } // end while depency

    return true;
  } // end of member function update



  /**
   * cache files
   *
   * @param int data_id data to be cached, if nothing is set, everything will be cached
   * @param bool depencies if set to true, it'll be cached recursivly
   * @access private
   */
  private function cacheFiles( $data_id = null, $depencies = true )
  {
    // set dir to generate contents
    static $backup;
    static $first;
    if (empty($backup)){
      $backup = $this->base_dir;
      $first = $data_id;
      $this->base_dir = $this->cache_dir;
    }

    if ($data_id === null) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT d.id AS data_id, d.type, d.name, l.id AS lang_id FROM ".ROSCMST_ENTRIES." d CROSS JOIN ".ROSCMST_LANGUAGES." l WHERE d.type = 'content' OR d.type = 'script' ORDER BY l.level DESC, l.name ASC");
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id AS data_id, type, name, :lang_id AS lang_id FROM ".ROSCMST_ENTRIES." WHERE id=:data_id AND type != 'system'");
      $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
      $stmt->bindParam('lang_id',$this->lang_id,PDO::PARAM_INT);
    }
    $stmt->execute();

    // prepare for usage in loop
      $stmt_more=&DBConnection::getInstance()->prepare("SELECT w.child_id, d.type, d.name FROM ".ROSCMST_DEPENCIES." w JOIN ".ROSCMST_ENTRIES." d ON w.child_id=d.id WHERE w.rev_id=:rev_id AND w.include IS TRUE AND d.type != 'script'");

    while ($data = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // change language only on top level
      if($data_id === null){
        $this->lang_id = $data['lang_id'];
      }

      $filename = $this->short[$data['type']].'_'.$data['name'].'.rcf';

      $revision = $this->getFrom($data['type'],$data['name']);

      // can I copy from standard language ?
      if (!$this->cloneFile(Language::getStandardId(),$data['lang_id'], $filename, $revision['lang_id'])) {

        // generate file
        $this->rev_id = $revision['id'];
        if (preg_match('/_([1-9][0-9]*)$/', $data['name'],$matches)) {
          $this->dynamic_num = $matches[1];
        }
        else {
          $this->dynamic_num = false;
        }
        
        // get content and replace [#cont_[%NAME%]] with [#cont_~pagename~]
        $content = preg_replace('/\[#cont_\[%Name%\]\]/i',$this->page_name, $revision['content']);

        // replace links
        $content = preg_replace_callback('/\[#link_([^][#[:space:]]+)\]/', array($this, 'replaceWithHyperlink'), $content);

        // do we care about depencies ?
        if ($depencies) {

        // process depencies first
          $stmt_more->bindParam('rev_id',$revision['id'],PDO::PARAM_INT);
          $stmt_more->execute();
          $depencies = $stmt_more->fetchAll(PDO::FETCH_ASSOC);
          foreach ($depencies as $depency) {

            // cache dependent entries first
            $depency_file = $data['lang_id'].'/'.$this->short[$depency['type']].'_'.$depency['name'].'.rcf';
            if (!file_exists($this->cache_dir.$depency_file) || $this->begin > date('Y-m-d H:i:s',filemtime($this->cache_dir.$depency_file))) {
              $this->cacheFiles($depency['child_id']);
            }

            // replace
            $content = '~'.$this->lang_id.'~'.str_replace('[#'.$this->short[$depency['type']].'_'.$depency['name'].']', $this->getCached(array(null, $this->short[$depency['type']].'_'.$depency['name'])), $content);
          }
        }
        $this->writeFile($data['lang_id'],$filename, $content);
      }
    } // end while

    // reset old build path
    if ($first == $data_id) {
      $this->base_dir = $backup;
      $backup = null;
    }
  } // end of member function cacheFiles



  /**
   * replaces [#roscms_* placeholder with their contents
   *
   * @param string content
   * @return string
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
      $content = str_replace('[#roscms_pagetitle]', Revision::getSText($this->rev_id, 'title'), $content); 
    }

    // replace roscms constants dependent from dynamic number
    else {
      $content = str_replace('[#roscms_filename]', $this->page_name.'_'.$this->dynamic_num.'.html', $content);
      $content = str_replace('[#roscms_pagename]', $this->page_name.'_'.$this->dynamic_num, $content); 
      $content = str_replace('[#roscms_pagetitle]', Revision::getSText($this->rev_id, 'title').' #'.$this->dynamic_num, $content); 
    }

    // get language information
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, name_short FROM ".ROSCMST_LANGUAGES." WHERE id = :lang LIMIT 1");
    $stmt->bindParam('lang',$this->lang_id,PDO::PARAM_INT);
    $stmt->execute();
    $lang=$stmt->fetchOnce(PDO::FETCH_ASSOC);

    // replace language information
    $content = str_replace('[#roscms_language]', $lang['name'], $content); 
    $content = str_replace('[#roscms_language_short]', $lang['name_short'], $content); 
    $content = str_replace('[#roscms_language_id]', $this->lang_id, $content);

    return $content;
  } // end of member function replaceRoscmsPlaceholder



  /**
   * replace [#link_ABC] with roscms link to ABC (normally called via preg_replace_callback)
   *
   * @param string[] matches
   * @return string
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

    // get revision
    $stmt=&DBConnection::getInstance()->prepare("SELECT r.id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id WHERE d.name = :name AND (d.type='page' OR d.type='dynamic') AND r.version > 0 LIMIT 1");
    $stmt->bindParam('name',$page_name,PDO::PARAM_STR);
    $stmt->execute();

    // get page extension
    $link_extension = Tag::getValue($stmt->fetchColumn(), 'extension', -1);
    if ($link_extension === false) {
      $link_extension = 'html';
    }

    // get page link
    return $raw_page_name.'.'.$link_extension;
  } // end of member function replaceWithHyperlink



  /**
   * returns an array with revision information & text
   * - if an localized version is available that one is returned
   * - otherwise the standard language version
   *
   * @param string type
   * @param string name
   * @return mixed[]
   * @access private
   */
  private function getFrom( $type, $name )
  {
    // get entry
    $stmt=&DBConnection::getInstance()->prepare("SELECT t.content, r.id, r.lang_id FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_REVISIONS." r ON r.data_id = d.id JOIN ".ROSCMST_TEXT." t ON t.rev_id = r.id WHERE d.name = :name AND d.type = :type AND r.version > 0 AND r.lang_id = :lang_id AND r.archive IS FALSE AND t.name = 'content' AND status='stable' LIMIT 1");
    $stmt->bindParam('name',$name,PDO::PARAM_STR);
    $stmt->bindParam('type',$type,PDO::PARAM_STR);
    $stmt->bindParam('lang_id',$this->lang_id,PDO::PARAM_INT);
    $stmt->execute();
    $revision=$stmt->fetch(PDO::FETCH_ASSOC);

    // check if depency not available
    if ($revision === false) {
      $stmt->bindParam('lang_id',Language::getStandardId(),PDO::PARAM_INT);
      $stmt->execute();
      $revision=$stmt->fetch(PDO::FETCH_ASSOC);
      
      if ($revision === false) {
        return false;
      }
    }
    return $revision;
  }



  /**
   * write a file and cares about if everything is right, and tries to create folders if needed
   *
   * @param string dir
   * @param string filename
   * @param string content
   * @return bool
   * @access private
   */
  private function writeFile( $dir, $filename, $content )
  {
    $dir .= '/';
    $file = $dir.$filename;

    // does not file exists or is content outdated
    if (!file_exists($this->base_dir.$file) || $this->begin > date('Y-m-d H:i:s',filemtime($this->base_dir.$file))) {

      // create folder, if necessary
      if(!is_dir($this->base_dir.$dir)) {
        mkdir($this->base_dir.$dir);
      }

      // schreiben
      $fh = @fopen($this->base_dir.$file, 'w');
      if ($fh !== false) {
        flock($fh, 2);
        $success = fwrite($fh, $content); 
        flock($fh, 3);
        fclose($fh);
        return $success;
      }
    }
    return false;
  } // end of member function writeFile



  /**
   * This function tries to copy a file from the standard language folder to another language folder
   *
   * @param string source_folder
   * @param string dest_folder
   * @param string filename
   * @param int content_lang
   * @return bool
   * @access private
   */
  private function cloneFile( $source_folder, $dest_folder, $filename, $content_lang )
  {
    // check, if language is different than standard and if requested file exists
    if ($this->lang_id != Language::getStandardId() && $content_lang == Language::getStandardId() && file_exists($this->base_dir.$source_folder.'/'.$filename)) {
      return @copy($this->base_dir.$source_folder.'/'.$filename, $this->base_dir.$dest_folder.'/'.$filename);
    }
    return false;
  } // end of member function cloneFile



  /**
   * get cached content (usually called via preg_replace_callback)
   *
   * @param string[] matches 
   * @return string
   * @access private
   */
  private function getCached( $matches )
  {
    // get cached content
    $fh = @fopen($this->cache_dir.$this->lang_id.'/'.$matches[1].'.rcf', 'r');
    if ($fh !== false){
      $content = fread($fh, filesize($this->cache_dir.$this->lang_id.'/'.$matches[1].'.rcf')+1); 
      fclose($fh);
    }

    // fail
    else {
      $content = '[#'.$matches[1].'--'.$this->cache_dir.$this->lang_id.'/'.$matches[1].'.rcf'.']';
    }

    return $content;
  } // end of member function getCached



  /**
   * executes code from a script
   *
   * @param string[] matches
   * @return string
   * @access private
   */
  private function evalScript( $matches )
  {  
    // get entry
    $revision=$this->getFrom('script',$matches[1]);

    // execute php code
    if( Tag::getValue($revision['id'], 'kind',-1) == 'php') {

      // catch output
      ob_start();

      // some vars for script usage;
      $roscms_dynamic_number = $this->dynamic_num;
      $roscms_lang_id = $this->lang_id;

      // execute code and return the output
      eval('?>'.$revision['content']);
      $content = ob_get_contents(); 
      ob_end_clean();

      // replace roscms links
      $content = preg_replace_callback('/\[#link_([^][#[:space:]]+)\]/', array($this, 'replaceWithHyperlink'), $content);
    }

    // no other script types supported -> return nothing
    else {
      $content = $revision['content'];
    }

    return $content;
  } // end of member function evalScript



} // end of Generator
?>
