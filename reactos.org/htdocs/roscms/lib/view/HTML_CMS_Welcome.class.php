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
 * class HTML_CMS_Welcome
 * 
 * @package html
 * @subpackage cms
 */
class HTML_CMS_Welcome extends HTML_CMS
{


  /**
   * setup branch info
   *
   * @access public
   */
  public function __construct( )
  {
    $this->branch = 'welcome';

    parent::__construct( );
  } // end of constructor



  protected function body( )
  {
    echo_strip('
      <ul style="font-weight: bold;">
        <li><a href="#web_news">RosCMS Website News</a></li>
        <li><a href="#web_news_langgroup">Translator Information</a></li>
      </ul>
      <br />
      <a name="web_news"></a><h3>');echo Entry::getContent('web_news', 'system', Language::getStandardId(), 'title', 'stext');echo_strip('</h3>
      <p style="font-weight: bold;">');echo Entry::getContent('web_news', 'system', Language::getStandardId(), 'heading', 'stext').'</p>'.
      Entry::getContent('web_news', 'system', Language::getStandardId(), 'content', 'text').'<br />';

      // request language specific welcome information
      $user_lang = ThisUser::getInstance()->language();
      if ($user_lang !== false) {
        echo_strip('
          <a name="web_news_langgroup"></a>
          <h3>Translator Information</h3>');

        // try to get content in local language, otherwise use standard language
        $content = Entry::getContent('web_news_langgroup', 'system',  $user_lang, 'content', 'text');
        if ($content == '') {
          $content = Entry::getContent('web_news_langgroup', 'system',  Language::getStandardId(), 'content', 'text');
        }
        echo $content;
      }

      // no language was set
      else {
        echo_strip('
          <h2>Please set your favorite language in the '.RosCMS::getInstance()->siteName().' profile settings.</h2>
          <p>This language will also be the default language to that you can translate content.</p>');
      }
    echo '<br />';
  } // end of member function body



} // end of HTML_CMS_Welcome
?>
