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

    protected $pdo;
    protected $params = array();
    protected $values = array();
    protected $result;
    protected $handle;
    protected $statement;


    public function __construct( $handle )
    {
      // inherit from PDO
      $this->pdo = $handle;
    }
    
    
    // replaces :param with data
    public function bindValue( $param, $data, $type = null )
    {
      $name = str_replace(':','',$param);
      $this->params[$name]=array($type,$data);
    }


    // replaces :param with data
    public function bindParam( $param, &$data, $type = null )
    {
      $name = str_replace(':','',$param);
      $this->params[$name]=array($type,&$data);
      echo  "$param, $data, $type\n\n";
    }
   
    
    // executes the current query
    public function execute()
    {
      $sql = $this->pdo->statement;
      if (count($this->params) > 0) {
        foreach ($this->params as $name=>$val){
          switch ($val[0]) {
            case PDO::PARAM_INT: $value=intval($val[1]); break;
            case PDO::PARAM_BOOL: $value=($val[1]!=null?'TRUE':'FALSE'); break;
            default:
            case PDO::PARAM_STR: $value="'".mysql_real_escape_string($val[1],$this->pdo->handle)."'"; break;
          }
          $sql = str_replace(':'.$name,$value,$sql);
        }
      }
      $this->result=mysql_query($sql);

      if (mysql_errno()) {
        switch ($this->pdo->attributes['errmode']) {
          case 'warning':
            echo '<b>Warning</b>'.mysql_error().' in statement &quot;'.$sql.'&quot;<br/>';
            break;
        }
      }
      return $this->result;
    }
    
    
    // fetches the results
    public function fetch( $type=PDO::FETCH_BOTH )
    {
      if ($this->result !== false) {
        return mysql_fetch_array($this->result,$type);
      }
      return false;
    }
    
    public function closeCursor( )
    {
      if ($this->result !== false) {
        return mysql_free_result($this->result);
      }
      return false;
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

  } // end of PDOStatement
}
?>
