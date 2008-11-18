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
if (!class_exists('PDOStatement')) {

  class PDOStatement
  {

    private $handle;
    private $dbhandle;
    private $statement;
    private $params;
    private $values;
    private $attributes;
    private $result;
    

    public function __construct( $handle, $dbhandle, $attributes, $statement )
    {
      // inherit from PDO
      $this->handle = $handle;
      $this->dbhandle = $dbhandle;
      $this->statement = $statement;
      $this->attributes = $attributes;
      
      $this->params = array();
      $this->values = array();
    }
    
    
    // replaces :param with data
    public function bindValue( $param, $data, $type = null )
    {
      $name = str_replace(':','',$param);
      switch ($type) {
        case PDO::PARAM_INT: $this->values[]=array('param'=>$name,'val'=>intval($data)); break;
        case PDO::PARAM_BOOL: $this->values[]=array('param'=>$name,'val'=>($data!=null?'TRUE':'FALSE')); break;
        default:
        case PDO::PARAM_STR: $this->values[]=array('param'=>$name,'val'=>"'".mysql_real_escape_string($data,$this->handle)."'"); break;
      }
    }


    // replaces :param with data
    public function bindParam( $param, &$data, $type = null )
    {
      $name = str_replace(':','',$param);
      switch ($type) {
        case PDO::PARAM_INT: $this->params[]=array('param'=>$name,'val'=>intval($data)); break;
        case PDO::PARAM_BOOL: $this->params[]=array('param'=>$name,'val'=>($data!=null?'TRUE':'FALSE')); break;
        default:
        case PDO::PARAM_STR: $this->params[]=array('param'=>$name,'val'=>"'".mysql_real_escape_string($data,$this->handle)."'"); break;
      }
    }
   
    
    // executes the current query
    public function execute()
    {
      $sql = $this->statement;
      
      foreach ($this->params as $pair){
        $sql = str_replace(':'.$pair['param'],$pair['val'],$sql);
      }
      foreach ($this->values as $pair){
        $sql = str_replace(':'.$pair['param'],$pair['val'],$sql);
      }
      $this->result=@mysql_query($sql,$this->handle);
      
      if (mysql_errno()) {
        switch (@$this->attributes['errmode']) {
          case 'warning':
            echo '<b>Warning</b>'.mysql_error().' in statement &quot;'.$sql.'&quot;<br/>';
            break;
        }
      }
      return $this->result;
    }
    
    
    // fetches the results
    public function fetch( $type=MYSQL_BOTH )
    {
      return mysql_fetch_array($this->result,$type);
    }
    
    public function closeCursor( )
    {
      return mysql_free_result($this->result);
    }
    
    // fetches the results
    public function fetchColumn( $offset = 0 )
    {
      $row = self::fetch(MYSQL_NUM);
      return $row[$offset];
    }
    
    
    // fetches all results
    public function fetchAll($type=MYSQL_BOTH)
    {
      $list=array();
      while ($new=mysql_fetch_array($this->result,$type)) {
        $list[]=$new;
      }
      return $list;
    }
    
  } // end of PDO
}
?>
