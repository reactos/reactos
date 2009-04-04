<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

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


class About extends HTML
{



  protected function body( )
  {
    global $RSDB_langres;
    global $RSDB_intern_index_php;

    echo '
      <h1><a href="'.$RSDB_intern_index_php.'?page=about">ReactOS Compatability Database</a> &gt; '.$RSDB_langres['TEXT_about_rsdb'].'</h1> 
      <h2>'.$RSDB_langres['TEXT_about_rsdb'].'</h2> 
      <p>'.$RSDB_langres['CONTENT_about_rsdb'].'</p>
      <p>'.$RSDB_langres['CONTENT_about_webteam'].'</p>
      <p>&nbsp;</p>
      <h3>'.$RSDB_langres['TEXT_credits'].'</h3>
      <p><strong>Klemens Friedl:</strong> '.$RSDB_langres['CONTENT_about_credits_frik85'].'</p>';
  } // end of member function body



} // end of About
