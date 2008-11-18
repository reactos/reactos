<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2008  Danny Götte <dangerground@web.de>

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


// The whole function is a hack around non native pdo availability
if (!class_exists('PDO')) {

  class PDO
  {
    const PARAM_STR  = 1;
    const PARAM_INT  = 2;
    const PARAM_BOOL = 3;

    const FETCH_ASSOC = MYSQL_ASSOC;
    const FETCH_BOTH  = MYSQL_BOTH;
    const FETCH_NUM   = MYSQL_NUM;
    
    const ATTR_ERRMODE = 1;
      const ERRMODE_WARNING = 1;
      
      
    const ATTR_STATEMENT_CLASS = 16;


    private $handle;
    private $statement;
    private $attributes = array();
    

    public function __construct($params, $user, $pass)
    {
      $params = preg_replace('/^([a-z0-9_\-]+):(.*)$/','$1%|%$2',$params);
      $settings = explode('%|%',$params);
      if ($settings[0] != 'mysql') die('Unsupported DB Driver:'.$settings[0]);
      $params = explode(';',$settings[1]);
      foreach ($params as $param){
        $pair = explode('=',$param);
        if ($pair[0]=='dbname') $name = $pair[1];
        if ($pair[0]=='host') $host = $pair[1];
      }
    
      $this->handle = @mysql_connect($host,$user,$pass);
      $this->dbhandle = @mysql_select_db($name,$this->handle);
      if ($this->dbhandle === false) {
        die('Can\'t select Database');
      }
    }
    
    // store statement
    public function prepare($sql)
    {
      return new DBStatement($this->handle, $this->dbhandle, $this->attributes, $sql);
    }

    // store statement
    public function quote($data, $type=null)
    {
      switch ($type) {
        case self::PARAM_INT: return intval($data); break;
        case self::PARAM_BOOL: return ($data!=null?'TRUE':'FALSE'); break;
        default:
        case self::PARAM_STR: return "'".mysql_real_escape_string($data,$this->handle)."'"; break;
      }
    }

    // 
    public function lastInsertId( $param = null )
    {
      return mysql_insert_id($this->handle);
    }
    
    
    // executes statement without return value
    public function exec( $sql )
    {
      $result=@mysql_query($sql,$this->handle);
      mysql_free_result($this->handle);
    }

    // set Attributes
    public function setAttribute($type, $value)
    {
      switch ($type){
        case self::ATTR_ERRMODE:
          switch ($value){
            case self::ERRMODE_WARNING:
              $this->attributes['errmode'] = 'warning';
              break;
          }
          break;
      }
    }
    
  } // end of PDO
}
?>
