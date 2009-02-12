<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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
 * class HTML
 * 
 * @package html
 */
abstract class HTML
{

  protected $page_start; // start time of page generation
  protected $title; 
  protected $css_files=array(); 
  protected $js_files=array(); 



  /**
   * set page title, start generation time counter and get page output
   *
   * @access public
   */
  public function __construct( $page_title = '' )
  {
    // get page title and register css files
    $this->title = $page_title;
    $this->register_css('style.css');

    // get output
    $this->build();
  } // end of constructor



  /**
   * output of the whole site
   *
   * @access private
   */
  private function build( )
  {
    $this->header();
    $this->body();
    $this->footer();
  } // end of member function build



  /**
   * includes the page itself without header + footer
   *
   * @access protected
   */
  abstract protected function body( );



  /**
   * output site header
   *
   * @access protected
   */
  protected function header( )
  {
    $config = &RosCMS::getInstance();

    // this page was generated in ...
    $roscms_gentime = explode(' ',microtime()); 
    $this->page_start = $roscms_gentime[1] + $roscms_gentime[0];

    // output header
    echo_strip( '
      <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
      <html lang="en">
      <head>
        <title>'.$config->systemBrand().' '.(($this->title!=='') ? '- '.$this->title : '').'</title>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
        <meta http-equiv="Pragma" content="no-cache" />
        <meta name="Copyright" content="ReactOS Foundation" />
        <meta name="generator" content="RosCMS" />
        <meta name="Content-language" content="en" />
        <meta name="Robots" content="noindex,nofollow" />
        <meta http-equiv="Content-Script-Type" content="text/javascript" />
        <meta http-equiv="Content-Style-Type" content="text/css" />
        <link rel="SHORTCUT ICON" href="../favicon.ico" />');

    // link css files (use register_css method)
    foreach($this->css_files as $file) {
      if ($file['condition'] === false) {
        echo '<link href="'.$config->pathRosCMS().'css/'.$file['name'].'" type="text/css" rel="stylesheet" />';
      }
      else {
        echo '<!--[if '.$file['condition'].']<link href="'.$config->pathRosCMS().'css/'.$file['name'].'" type="text/css" rel="stylesheet" /><![endif]-->';
      }
    }

    // link js files (use register_js method)
    foreach($this->js_files as $file) {
      echo '<script src="'.$config->pathRosCMS().'js/'.$file.'" type="text/javascript"></script>';
    }

    //@TODO remove those static links from here
    echo_strip('
      </head>
      <body>
      <div id="top">
        <div id="topMenu"> 
          <a href="'.$config->pathGenerated().'?page=index">Home</a> <span>|</span>
          <a href="'.$config->pathGenerated().'?page=about">Info</a> <span>|</span>
          <a href="'.$config->pathGenerated().'?page=community">Community</a> <span>|</span>
          <a href="'.$config->pathGenerated().'?page=dev">Developement</a> <span>|</span>
          <a href="'.$config->pathInstance().'?page=user">myRosCMS</a>
        </div>
      </div>');
  } // end of member function header



  /**
   * output site footer
   *
   * @access protected
   */
  protected function footer( )
  {
    // This page was generated in ...
    $gentime = explode(' ',microtime()); 
    $page_end = $gentime[1] + $gentime[0]; 
    $page_time = number_format($page_end - $this->page_start, 4, '.', ''); 

    // footer information
    echo_strip('
      <hr />
      <div id="footer" style="text-align:center;">
        '.RosCMS::getInstance()->systemBrand().' - (c) 2005-2008 Klemens Friedl, Danny G&ouml;tte<br />
        <br />
        Page generated in '.$page_time.' seconds
      </div>
      </body>
      </html>');
  } // end of member function footer



  /**
   * register a css file to be included in the header
   *
   * @param string name path & filename to a css file
   * @param bool|string condition conditional statement (used by IE) to e.g. include sheets only for specific IE versions
   * @access protected
   */
  protected function register_css( $name, $condition = false )
  {
    $this->css_files[] = array('name'=>$name, 'condition'=>$condition);
  } // end of member function register_css



  /**
   * register a javascript file to be included in the header
   *
   * @param string name path & filename to a javascript file
   * @access protected
   */
  protected function register_js( $name )
  {
    $this->js_files[] = $name;
  } // end of member function register_js



} // end of HTML
?>
