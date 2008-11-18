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
 * class Editor
 * 
 */
abstract class Editor
{
  protected $data_id;
  protected $rev_id;



  /**
   * constructor
   *
   * @access public
   */
  public function __construct( $data_id, $rev_id, $action = null )
  {
    require('login.php');
    $this->preventCaching();
    if (!empty($data_id) && !empty($rev_id)) {
      $this->data_id = $data_id;
      $this->rev_id = $rev_id;
    }
    if ($action != null) {
      $this->evalAction($action);
    }
    else {
      $this->performDefaultAction();
    }
  }


  /**
   *
   *
   * @access private
   */
  protected function preventCaching()
  {
    header('Content-type: text/html');
    header('Expires: Sun, 28 Jul 1996 05:00:00 GMT'); // Date in the past
    header('Last-Modified: '.gmdate('D, d M Y H:i:s').' GMT'); // always modified
    header('Cache-Control: no-store, no-cache, must-revalidate'); // HTTP/1.1
    header('Cache-Control: post-check=0, pre-check=0', false);
    header('Pragma: no-cache'); // HTTP/1.0
  }


  abstract protected function evalAction( $action );
  abstract protected function performDefaultAction();

} // end of Editor
?>
