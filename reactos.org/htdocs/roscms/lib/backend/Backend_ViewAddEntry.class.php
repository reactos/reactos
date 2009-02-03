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
 * class Backend_ViewAddEntry
 * 
 * @package Branch_Website
 * @subpackage Backend
 */
class Backend_ViewAddEntry extends Backend
{



  // types of new entries
  const SINGLE   = 0;
  const DYNAMIC  = 1;
  const TEMPLATE = 2;

  // 
  private $data_id;
  private $rev_id;



  /**
   * constructor
   *
   * @access public
   */
  public function __construct( )
  {
    // prevent caching, set content header for text
    parent::__construct();

    if (!ThisUser::getInstance()->hasAccess('new_entry')) {
      die('No rights to add new entries.');
    }

    $this->evalAction();
  } // end of constructor



  /**
   *
   *
   * @access private
   */
  private function evalAction(  )
  {
    switch ($_GET['action']) {

      // single entry - save entry
      case 'newentry': 
        $rev_id = Entry::add($_GET['name'], $_GET['data_type']);
        new Backend_ViewEditor($rev_id);
        break;

      // dynamic entry - save entry
      case 'newdynamic':
        $rev_id = Entry::add($_GET['name'], 'content', null, true);
        new Backend_ViewEditor($rev_id);
        break;

      // page & content - save entry
      case 'newcombo': 
        Entry::add($_GET['name'], 'page');
        $rev_id = Entry::add($_GET['name'], 'content', $_GET['template']);
        new Backend_ViewEditor($rev_id);
        break;

      // create entry - show interface
      case 'dialog':
      default:

        // get type of entry
        switch ($_GET['tab']) {
          case 'dynamic':
            $this->show(self::DYNAMIC);
            break;
          case 'template':
            $this->show(self::TEMPLATE);
            break;
          case 'single':
          default:
            $this->show(self::SINGLE);
            break;
        } // end switch type
        break;
      
    } // end switch action
  } // end of member function evalAction



  /**
   *
   *
   * @access private
   */
  private function show( $mode = self::SINGLE )
  {
    echo_strip('
      <div id="frmadd" class="editor" style="border-bottom: 1px solid #bbb; border-right: 1px solid #bbb; background: #FFFFFF none repeat scroll 0%;">
        <div style="margin:10px;">
          <div class="detailbody">
            <div class="detailmenubody" id="newentrymenu">');

    // is single
    if ($mode == self::SINGLE) {
      echo '<strong>Single Entry</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changeNewEntryTab('single')".'">Single Entry</span>';
    }
    echo '&nbsp;|&nbsp;';

    // is dynamic
    if ($mode == self::DYNAMIC) {
      echo '<strong>Dynamic Entry</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changeNewEntryTab('dynamic')".'">Dynamic Entry</span>';
    }
    echo '&nbsp;|&nbsp;';

    // is page & content
    if ($mode == self::TEMPLATE) {
      echo '<strong>Page &amp; Content</strong>';
    }
    else {
      echo '<span class="detailmenu" onclick="'."changeNewEntryTab('template')".'">Page &amp; Content</span>';
    }

    echo_strip('
        </div>
      </div>');

    // is single
    switch ($mode) {

      // is single
      case self::SINGLE:
        echo_strip('
          <br />
          <label for="txtaddentryname">Name</label>
          <input type="text" id="txtaddentryname" name="txtaddentryname" />
          <br />
          <br />
          <br />
          <label for="txtaddentrytype">Type</label>
          <select id="txtaddentrytype" name="txtaddentrytype">
            <option value="page">Page</option>
            <option value="content">Content</option>
            <option value="template">Template</option>
            <option value="script">Script</option>'.(ThisUser::getInstance()->hasAccess('dynamic_pages') ? '
            <option value="dynamic">Dynamic Page</option>' : '').'
          </select>');
        break;

      // is dynamic
      case self::DYNAMIC:
        echo_strip('
          <br />
          <label for="txtadddynsource">Source</label>
          <select id="txtadddynsource" name="txtadddynsource">');

        // list dynamic pages
        $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_ENTRIES." WHERE type='dynamic' ORDER BY name ASC");
        $stmt->execute();
        while($data=$stmt->fetch(PDO::FETCH_ASSOC)) {
          echo '<option value="'.$data['id'].'">'.$data['name'].'</option>';
        }
        
        echo '</select>';
        break;

      // is page & content
      case self::TEMPLATE:
        echo_strip('
          <br />
          <label for="txtaddentryname3">Name</label>
          <input type="text" id="txtaddentryname3" name="txtaddentryname3" />
          <br />
          <br />
          <label for="txtaddtemplate">Template</label>
          <select id="txtaddtemplate" name="txtaddtemplate">
            <option value="none" selected="selected">no template</option>');

        // select templates
        $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT d.name FROM ".ROSCMST_ENTRIES." d ON r.data_id = d.id WHERE r.status = 'stable' AND r.archive IS FALSE AND d.type = 'template' ORDER BY d.name ASC");
        $stmt->execute();
        while ($templates = $stmt->fetch(PDO::FETCH_ASSOC)) {
          echo '<option value="'. $templates['name'] .'">'. $templates['name'] .'</option>';
        }
        
        echo '</select>';
        break;
    }

    echo_strip('
          <br />
          <br />
          <button type="button" onclick="'."createNewEntry(".$mode.")".'">Create</button>
        </div>
      </div>');
  } // end of member function showAddEntry



} // end of Backend_ViewAddEntry
?>
