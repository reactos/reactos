<?php
/**
*
* @package dbal
* @version $Id: db_tools.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2007 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

/**
* Database Tools for handling cross-db actions such as altering columns, etc.
* Currently not supported is returning SQL for creating tables.
*
* @package dbal
* @note currently not used within phpBB3, but may be utilized later.
*/
class phpbb_db_tools
{
	/**
	* Current sql layer
	*/
	var $sql_layer = '';

	var $dbms_type_map = array(
		'mysql_41'	=> array(
			'INT:'		=> 'int(%d)',
			'BINT'		=> 'bigint(20)',
			'UINT'		=> 'mediumint(8) UNSIGNED',
			'UINT:'		=> 'int(%d) UNSIGNED',
			'TINT:'		=> 'tinyint(%d)',
			'USINT'		=> 'smallint(4) UNSIGNED',
			'BOOL'		=> 'tinyint(1) UNSIGNED',
			'VCHAR'		=> 'varchar(255)',
			'VCHAR:'	=> 'varchar(%d)',
			'CHAR:'		=> 'char(%d)',
			'XSTEXT'	=> 'text',
			'XSTEXT_UNI'=> 'varchar(100)',
			'STEXT'		=> 'text',
			'STEXT_UNI'	=> 'varchar(255)',
			'TEXT'		=> 'text',
			'TEXT_UNI'	=> 'text',
			'MTEXT'		=> 'mediumtext',
			'MTEXT_UNI'	=> 'mediumtext',
			'TIMESTAMP'	=> 'int(11) UNSIGNED',
			'DECIMAL'	=> 'decimal(5,2)',
			'DECIMAL:'	=> 'decimal(%d,2)',
			'PDECIMAL'	=> 'decimal(6,3)',
			'PDECIMAL:'	=> 'decimal(%d,3)',
			'VCHAR_UNI'	=> 'varchar(255)',
			'VCHAR_UNI:'=> 'varchar(%d)',
			'VCHAR_CI'	=> 'varchar(255)',
			'VARBINARY'	=> 'varbinary(255)',
		),

		'mysql_40'	=> array(
			'INT:'		=> 'int(%d)',
			'BINT'		=> 'bigint(20)',
			'UINT'		=> 'mediumint(8) UNSIGNED',
			'UINT:'		=> 'int(%d) UNSIGNED',
			'TINT:'		=> 'tinyint(%d)',
			'USINT'		=> 'smallint(4) UNSIGNED',
			'BOOL'		=> 'tinyint(1) UNSIGNED',
			'VCHAR'		=> 'varbinary(255)',
			'VCHAR:'	=> 'varbinary(%d)',
			'CHAR:'		=> 'binary(%d)',
			'XSTEXT'	=> 'blob',
			'XSTEXT_UNI'=> 'blob',
			'STEXT'		=> 'blob',
			'STEXT_UNI'	=> 'blob',
			'TEXT'		=> 'blob',
			'TEXT_UNI'	=> 'blob',
			'MTEXT'		=> 'mediumblob',
			'MTEXT_UNI'	=> 'mediumblob',
			'TIMESTAMP'	=> 'int(11) UNSIGNED',
			'DECIMAL'	=> 'decimal(5,2)',
			'DECIMAL:'	=> 'decimal(%d,2)',
			'PDECIMAL'	=> 'decimal(6,3)',
			'PDECIMAL:'	=> 'decimal(%d,3)',
			'VCHAR_UNI'	=> 'blob',
			'VCHAR_UNI:'=> array('varbinary(%d)', 'limit' => array('mult', 3, 255, 'blob')),
			'VCHAR_CI'	=> 'blob',
			'VARBINARY'	=> 'varbinary(255)',
		),

		'firebird'	=> array(
			'INT:'		=> 'INTEGER',
			'BINT'		=> 'DOUBLE PRECISION',
			'UINT'		=> 'INTEGER',
			'UINT:'		=> 'INTEGER',
			'TINT:'		=> 'INTEGER',
			'USINT'		=> 'INTEGER',
			'BOOL'		=> 'INTEGER',
			'VCHAR'		=> 'VARCHAR(255) CHARACTER SET NONE',
			'VCHAR:'	=> 'VARCHAR(%d) CHARACTER SET NONE',
			'CHAR:'		=> 'CHAR(%d) CHARACTER SET NONE',
			'XSTEXT'	=> 'BLOB SUB_TYPE TEXT CHARACTER SET NONE',
			'STEXT'		=> 'BLOB SUB_TYPE TEXT CHARACTER SET NONE',
			'TEXT'		=> 'BLOB SUB_TYPE TEXT CHARACTER SET NONE',
			'MTEXT'		=> 'BLOB SUB_TYPE TEXT CHARACTER SET NONE',
			'XSTEXT_UNI'=> 'VARCHAR(100) CHARACTER SET UTF8',
			'STEXT_UNI'	=> 'VARCHAR(255) CHARACTER SET UTF8',
			'TEXT_UNI'	=> 'BLOB SUB_TYPE TEXT CHARACTER SET UTF8',
			'MTEXT_UNI'	=> 'BLOB SUB_TYPE TEXT CHARACTER SET UTF8',
			'TIMESTAMP'	=> 'INTEGER',
			'DECIMAL'	=> 'DOUBLE PRECISION',
			'DECIMAL:'	=> 'DOUBLE PRECISION',
			'PDECIMAL'	=> 'DOUBLE PRECISION',
			'PDECIMAL:'	=> 'DOUBLE PRECISION',
			'VCHAR_UNI'	=> 'VARCHAR(255) CHARACTER SET UTF8',
			'VCHAR_UNI:'=> 'VARCHAR(%d) CHARACTER SET UTF8',
			'VCHAR_CI'	=> 'VARCHAR(255) CHARACTER SET UTF8',
			'VARBINARY'	=> 'CHAR(255) CHARACTER SET NONE',
		),

		'mssql'		=> array(
			'INT:'		=> '[int]',
			'BINT'		=> '[float]',
			'UINT'		=> '[int]',
			'UINT:'		=> '[int]',
			'TINT:'		=> '[int]',
			'USINT'		=> '[int]',
			'BOOL'		=> '[int]',
			'VCHAR'		=> '[varchar] (255)',
			'VCHAR:'	=> '[varchar] (%d)',
			'CHAR:'		=> '[char] (%d)',
			'XSTEXT'	=> '[varchar] (1000)',
			'STEXT'		=> '[varchar] (3000)',
			'TEXT'		=> '[varchar] (8000)',
			'MTEXT'		=> '[text]',
			'XSTEXT_UNI'=> '[varchar] (100)',
			'STEXT_UNI'	=> '[varchar] (255)',
			'TEXT_UNI'	=> '[varchar] (4000)',
			'MTEXT_UNI'	=> '[text]',
			'TIMESTAMP'	=> '[int]',
			'DECIMAL'	=> '[float]',
			'DECIMAL:'	=> '[float]',
			'PDECIMAL'	=> '[float]',
			'PDECIMAL:'	=> '[float]',
			'VCHAR_UNI'	=> '[varchar] (255)',
			'VCHAR_UNI:'=> '[varchar] (%d)',
			'VCHAR_CI'	=> '[varchar] (255)',
			'VARBINARY'	=> '[varchar] (255)',
		),

		'oracle'	=> array(
			'INT:'		=> 'number(%d)',
			'BINT'		=> 'number(20)',
			'UINT'		=> 'number(8)',
			'UINT:'		=> 'number(%d)',
			'TINT:'		=> 'number(%d)',
			'USINT'		=> 'number(4)',
			'BOOL'		=> 'number(1)',
			'VCHAR'		=> 'varchar2(255)',
			'VCHAR:'	=> 'varchar2(%d)',
			'CHAR:'		=> 'char(%d)',
			'XSTEXT'	=> 'varchar2(1000)',
			'STEXT'		=> 'varchar2(3000)',
			'TEXT'		=> 'clob',
			'MTEXT'		=> 'clob',
			'XSTEXT_UNI'=> 'varchar2(300)',
			'STEXT_UNI'	=> 'varchar2(765)',
			'TEXT_UNI'	=> 'clob',
			'MTEXT_UNI'	=> 'clob',
			'TIMESTAMP'	=> 'number(11)',
			'DECIMAL'	=> 'number(5, 2)',
			'DECIMAL:'	=> 'number(%d, 2)',
			'PDECIMAL'	=> 'number(6, 3)',
			'PDECIMAL:'	=> 'number(%d, 3)',
			'VCHAR_UNI'	=> 'varchar2(765)',
			'VCHAR_UNI:'=> array('varchar2(%d)', 'limit' => array('mult', 3, 765, 'clob')),
			'VCHAR_CI'	=> 'varchar2(255)',
			'VARBINARY'	=> 'raw(255)',
		),

		'sqlite'	=> array(
			'INT:'		=> 'int(%d)',
			'BINT'		=> 'bigint(20)',
			'UINT'		=> 'INTEGER UNSIGNED', //'mediumint(8) UNSIGNED',
			'UINT:'		=> 'INTEGER UNSIGNED', // 'int(%d) UNSIGNED',
			'TINT:'		=> 'tinyint(%d)',
			'USINT'		=> 'INTEGER UNSIGNED', //'mediumint(4) UNSIGNED',
			'BOOL'		=> 'INTEGER UNSIGNED', //'tinyint(1) UNSIGNED',
			'VCHAR'		=> 'varchar(255)',
			'VCHAR:'	=> 'varchar(%d)',
			'CHAR:'		=> 'char(%d)',
			'XSTEXT'	=> 'text(65535)',
			'STEXT'		=> 'text(65535)',
			'TEXT'		=> 'text(65535)',
			'MTEXT'		=> 'mediumtext(16777215)',
			'XSTEXT_UNI'=> 'text(65535)',
			'STEXT_UNI'	=> 'text(65535)',
			'TEXT_UNI'	=> 'text(65535)',
			'MTEXT_UNI'	=> 'mediumtext(16777215)',
			'TIMESTAMP'	=> 'INTEGER UNSIGNED', //'int(11) UNSIGNED',
			'DECIMAL'	=> 'decimal(5,2)',
			'DECIMAL:'	=> 'decimal(%d,2)',
			'PDECIMAL'	=> 'decimal(6,3)',
			'PDECIMAL:'	=> 'decimal(%d,3)',
			'VCHAR_UNI'	=> 'varchar(255)',
			'VCHAR_UNI:'=> 'varchar(%d)',
			'VCHAR_CI'	=> 'varchar(255)',
			'VARBINARY'	=> 'blob',
		),

		'postgres'	=> array(
			'INT:'		=> 'INT4',
			'BINT'		=> 'INT8',
			'UINT'		=> 'INT4', // unsigned
			'UINT:'		=> 'INT4', // unsigned
			'USINT'		=> 'INT2', // unsigned
			'BOOL'		=> 'INT2', // unsigned
			'TINT:'		=> 'INT2',
			'VCHAR'		=> 'varchar(255)',
			'VCHAR:'	=> 'varchar(%d)',
			'CHAR:'		=> 'char(%d)',
			'XSTEXT'	=> 'varchar(1000)',
			'STEXT'		=> 'varchar(3000)',
			'TEXT'		=> 'varchar(8000)',
			'MTEXT'		=> 'TEXT',
			'XSTEXT_UNI'=> 'varchar(100)',
			'STEXT_UNI'	=> 'varchar(255)',
			'TEXT_UNI'	=> 'varchar(4000)',
			'MTEXT_UNI'	=> 'TEXT',
			'TIMESTAMP'	=> 'INT4', // unsigned
			'DECIMAL'	=> 'decimal(5,2)',
			'DECIMAL:'	=> 'decimal(%d,2)',
			'PDECIMAL'	=> 'decimal(6,3)',
			'PDECIMAL:'	=> 'decimal(%d,3)',
			'VCHAR_UNI'	=> 'varchar(255)',
			'VCHAR_UNI:'=> 'varchar(%d)',
			'VCHAR_CI'	=> 'varchar_ci',
			'VARBINARY'	=> 'bytea',
		),
	);

	// A list of types being unsigned for better reference in some db's
	var $unsigned_types = array('UINT', 'UINT:', 'USINT', 'BOOL', 'TIMESTAMP');
	var $supported_dbms = array('firebird', 'mssql', 'mysql_40', 'mysql_41', 'oracle', 'postgres', 'sqlite');

	/**
	* Set this to true if you only want to return the 'to-be-executed' SQL statement(s) (as an array).
	*/
	var $return_statements = false;

	/**
	*/
	function phpbb_db_tools(&$db)
	{
		$this->db = $db;

		// Determine mapping database type
		switch ($this->db->sql_layer)
		{
			case 'mysql':
				$this->sql_layer = 'mysql_40';
			break;

			case 'mysql4':
				if (version_compare($this->db->mysql_version, '4.1.3', '>='))
				{
					$this->sql_layer = 'mysql_41';
				}
				else
				{
					$this->sql_layer = 'mysql_40';
				}
			break;

			case 'mysqli':
				$this->sql_layer = 'mysql_41';
			break;

			case 'mssql':
			case 'mssql_odbc':
				$this->sql_layer = 'mssql';
			break;

			default:
				$this->sql_layer = $this->db->sql_layer;
			break;
		}
	}

	/**
	* Handle passed database update array.
	* Expected structure...
	* Key being one of the following
	*	change_columns: Column changes (only type, not name)
	*	add_columns: Add columns to a table
	*	drop_keys: Dropping keys
	*	drop_columns: Removing/Dropping columns
	*	add_primary_keys: adding primary keys
	*	add_unique_index: adding an unique index
	*	add_index: adding an index
	*
	* The values are in this format:
	*		{TABLE NAME}		=> array(
	*			{COLUMN NAME}		=> array({COLUMN TYPE}, {DEFAULT VALUE}, {OPTIONAL VARIABLES}),
	*			{KEY/INDEX NAME}	=> array({COLUMN NAMES}),
	*		)
	*
	* For more information have a look at /develop/create_schema_files.php (only available through CVS)
	*/
	function perform_schema_changes($schema_changes)
	{
		if (empty($schema_changes))
		{
			return;
		}

		$statements = array();

		// Change columns?
		if (!empty($schema_changes['change_columns']))
		{
			foreach ($schema_changes['change_columns'] as $table => $columns)
			{
				foreach ($columns as $column_name => $column_data)
				{
					$result = $this->sql_column_change($table, $column_name, $column_data);

					if ($this->return_statements)
					{
						$statements = array_merge($statements, $result);
					}
				}
			}
		}

		// Add columns?
		if (!empty($schema_changes['add_columns']))
		{
			foreach ($schema_changes['add_columns'] as $table => $columns)
			{
				foreach ($columns as $column_name => $column_data)
				{
					// Only add the column if it does not exist yet
					if (!$this->sql_column_exists($table, $column_name))
					{
						$result = $this->sql_column_add($table, $column_name, $column_data);

						if ($this->return_statements)
						{
							$statements = array_merge($statements, $result);
						}
					}
				}
			}
		}

		// Remove keys?
		if (!empty($schema_changes['drop_keys']))
		{
			foreach ($schema_changes['drop_keys'] as $table => $indexes)
			{
				foreach ($indexes as $index_name)
				{
					$result = $this->sql_index_drop($table, $index_name);

					if ($this->return_statements)
					{
						$statements = array_merge($statements, $result);
					}
				}
			}
		}

		// Drop columns?
		if (!empty($schema_changes['drop_columns']))
		{
			foreach ($schema_changes['drop_columns'] as $table => $columns)
			{
				foreach ($columns as $column)
				{
					$result = $this->sql_column_remove($table, $column);

					if ($this->return_statements)
					{
						$statements = array_merge($statements, $result);
					}
				}
			}
		}

		// Add primary keys?
		if (!empty($schema_changes['add_primary_keys']))
		{
			foreach ($schema_changes['add_primary_keys'] as $table => $columns)
			{
				$result = $this->sql_create_primary_key($table, $columns);

				if ($this->return_statements)
				{
					$statements = array_merge($statements, $result);
				}
			}
		}

		// Add unqiue indexes?
		if (!empty($schema_changes['add_unique_index']))
		{
			foreach ($schema_changes['add_unique_index'] as $table => $index_array)
			{
				foreach ($index_array as $index_name => $column)
				{
					$result = $this->sql_create_unique_index($table, $index_name, $column);

					if ($this->return_statements)
					{
						$statements = array_merge($statements, $result);
					}
				}
			}
		}

		// Add indexes?
		if (!empty($schema_changes['add_index']))
		{
			foreach ($schema_changes['add_index'] as $table => $index_array)
			{
				foreach ($index_array as $index_name => $column)
				{
					$result = $this->sql_create_index($table, $index_name, $column);

					if ($this->return_statements)
					{
						$statements = array_merge($statements, $result);
					}
				}
			}
		}

		if ($this->return_statements)
		{
			return $statements;
		}
	}

	/**
	* Check if a specified column exist
	* @return bool True if column exists, else false
	*/
	function sql_column_exists($table, $column_name)
	{
		switch ($this->sql_layer)
		{
			case 'mysql_40':
			case 'mysql_41':

				$sql = "SHOW COLUMNS FROM $table";
				$result = $this->db->sql_query($sql);

				while ($row = $this->db->sql_fetchrow($result))
				{
					// lower case just in case
					if (strtolower($row['Field']) == $column_name)
					{
						$this->db->sql_freeresult($result);
						return true;
					}
				}
				$this->db->sql_freeresult($result);
				return false;
			break;

			// PostgreSQL has a way of doing this in a much simpler way but would
			// not allow us to support all versions of PostgreSQL
			case 'postgres':
				$sql = "SELECT a.attname
					FROM pg_class c, pg_attribute a
					WHERE c.relname = '{$table}'
						AND a.attnum > 0
						AND a.attrelid = c.oid";
				$result = $this->db->sql_query($sql);
				while ($row = $this->db->sql_fetchrow($result))
				{
					// lower case just in case
					if (strtolower($row['attname']) == $column_name)
					{
						$this->db->sql_freeresult($result);
						return true;
					}
				}
				$this->db->sql_freeresult($result);

				return false;
			break;

			// same deal with PostgreSQL, we must perform more complex operations than
			// we technically could
			case 'mssql':
				$sql = "SELECT c.name
					FROM syscolumns c
					LEFT JOIN sysobjects o ON c.id = o.id
					WHERE o.name = '{$table}'";
				$result = $this->db->sql_query($sql);
				while ($row = $this->db->sql_fetchrow($result))
				{
					// lower case just in case
					if (strtolower($row['name']) == $column_name)
					{
						$this->db->sql_freeresult($result);
						return true;
					}
				}
				$this->db->sql_freeresult($result);
				return false;
			break;

			case 'oracle':
				$sql = "SELECT column_name
					FROM user_tab_columns
					WHERE table_name = '{$table}'";
				$result = $this->db->sql_query($sql);
				while ($row = $this->db->sql_fetchrow($result))
				{
					// lower case just in case
					if (strtolower($row['column_name']) == $column_name)
					{
						$this->db->sql_freeresult($result);
						return true;
					}
				}
				$this->db->sql_freeresult($result);
				return false;
			break;

			case 'firebird':
				$sql = "SELECT RDB\$FIELD_NAME as FNAME
					FROM RDB\$RELATION_FIELDS
					WHERE RDB\$RELATION_NAME = '{$table}'";
				$result = $this->db->sql_query($sql);
				while ($row = $this->db->sql_fetchrow($result))
				{
					// lower case just in case
					if (strtolower($row['fname']) == $column_name)
					{
						$this->db->sql_freeresult($result);
						return true;
					}
				}
				$this->db->sql_freeresult($result);
				return false;
			break;

			// ugh, SQLite
			case 'sqlite':
				$sql = "SELECT sql
					FROM sqlite_master
					WHERE type = 'table'
						AND name = '{$table}'";
				$result = $this->db->sql_query($sql);

				if (!$result)
				{
					return false;
				}

				$row = $this->db->sql_fetchrow($result);
				$this->db->sql_freeresult($result);

				preg_match('#\((.*)\)#s', $row['sql'], $matches);

				$cols = trim($matches[1]);
				$col_array = preg_split('/,(?![\s\w]+\))/m', $cols);

				foreach ($col_array as $declaration)
				{
					$entities = preg_split('#\s+#', trim($declaration));
					if ($entities[0] == 'PRIMARY')
					{
						continue;
					}

					if (strtolower($entities[0]) == $column_name)
					{
						return true;
					}
				}
				return false;
			break;
		}
	}

	/**
	* Private method for performing sql statements (either execute them or return them)
	* @private
	*/
	function _sql_run_sql($statements)
	{
		if ($this->return_statements)
		{
			return $statements;
		}

		// We could add error handling here...
		foreach ($statements as $sql)
		{
			if ($sql === 'begin')
			{
				$this->db->sql_transaction('begin');
			}
			else if ($sql === 'commit')
			{
				$this->db->sql_transaction('commit');
			}
			else
			{
				$this->db->sql_query($sql);
			}
		}

		return true;
	}

	/**
	* Function to prepare some column information for better usage
	* @private
	*/
	function sql_prepare_column_data($table_name, $column_name, $column_data)
	{
		// Get type
		if (strpos($column_data[0], ':') !== false)
		{
			list($orig_column_type, $column_length) = explode(':', $column_data[0]);

			if (!is_array($this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']))
			{
				$column_type = sprintf($this->dbms_type_map[$this->sql_layer][$orig_column_type . ':'], $column_length);
			}
			else
			{
				if (isset($this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']['rule']))
				{
					switch ($this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']['rule'][0])
					{
						case 'div':
							$column_length /= $this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']['rule'][1];
							$column_length = ceil($column_length);
							$column_type = sprintf($this->dbms_type_map[$this->sql_layer][$orig_column_type . ':'][0], $column_length);
						break;
					}
				}

				if (isset($this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']['limit']))
				{
					switch ($this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']['limit'][0])
					{
						case 'mult':
							$column_length *= $this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']['limit'][1];
							if ($column_length > $this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']['limit'][2])
							{
								$column_type = $this->dbms_type_map[$this->sql_layer][$orig_column_type . ':']['limit'][3];
							}
							else
							{
								$column_type = sprintf($this->dbms_type_map[$this->sql_layer][$orig_column_type . ':'][0], $column_length);
							}
						break;
					}
				}
			}
			$orig_column_type .= ':';
		}
		else
		{
			$orig_column_type = $column_data[0];
			$column_type = $this->dbms_type_map[$this->sql_layer][$column_data[0]];
		}

		// Adjust default value if db-dependant specified
		if (is_array($column_data[1]))
		{
			$column_data[1] = (isset($column_data[1][$this->sql_layer])) ? $column_data[1][$this->sql_layer] : $column_data[1]['default'];
		}

		$sql = '';

		$return_array = array();

		switch ($this->sql_layer)
		{
			case 'firebird':
				$sql .= " {$column_type} ";

				if (!is_null($column_data[1]))
				{
					$sql .= 'DEFAULT ' . ((is_numeric($column_data[1])) ? $column_data[1] : "'{$column_data[1]}'") . ' ';
				}

				$sql .= 'NOT NULL';

				// This is a UNICODE column and thus should be given it's fair share
				if (preg_match('/^X?STEXT_UNI|VCHAR_(CI|UNI:?)/', $column_data[0]))
				{
					$sql .= ' COLLATE UNICODE';
				}

			break;

			case 'mssql':
				$sql .= " {$column_type} ";
				$sql_default = " {$column_type} ";

				// For adding columns we need the default definition
				if (!is_null($column_data[1]))
				{
					// For hexadecimal values do not use single quotes
					if (strpos($column_data[1], '0x') === 0)
					{
						$sql_default .= 'DEFAULT (' . $column_data[1] . ') ';
					}
					else
					{
						$sql_default .= 'DEFAULT (' . ((is_numeric($column_data[1])) ? $column_data[1] : "'{$column_data[1]}'") . ') ';
					}
				}

				$sql .= 'NOT NULL';
				$sql_default .= 'NOT NULL';

				$return_array['column_type_sql_default'] = $sql_default;
			break;

			case 'mysql_40':
			case 'mysql_41':
				$sql .= " {$column_type} ";

				// For hexadecimal values do not use single quotes
				if (!is_null($column_data[1]) && substr($column_type, -4) !== 'text' && substr($column_type, -4) !== 'blob')
				{
					$sql .= (strpos($column_data[1], '0x') === 0) ? "DEFAULT {$column_data[1]} " : "DEFAULT '{$column_data[1]}' ";
				}
				$sql .= 'NOT NULL';

				if (isset($column_data[2]))
				{
					if ($column_data[2] == 'auto_increment')
					{
						$sql .= ' auto_increment';
					}
					else if ($this->sql_layer === 'mysql_41' && $column_data[2] == 'true_sort')
					{
						$sql .= ' COLLATE utf8_unicode_ci';
					}
				}

			break;

			case 'oracle':
				$sql .= " {$column_type} ";
				$sql .= (!is_null($column_data[1])) ? "DEFAULT '{$column_data[1]}' " : '';

				// In Oracle empty strings ('') are treated as NULL.
				// Therefore in oracle we allow NULL's for all DEFAULT '' entries
				// Oracle does not like setting NOT NULL on a column that is already NOT NULL (this happens only on number fields)
				if (preg_match('/number/i', $column_type))
				{
					$sql .= ($column_data[1] === '') ? '' : 'NOT NULL';
				}
			break;

			case 'postgres':
				$return_array['column_type'] = $column_type;

				$sql .= " {$column_type} ";

				if (isset($column_data[2]) && $column_data[2] == 'auto_increment')
				{
					$default_val = "nextval('{$table_name}_seq')";
				}
				else if (!is_null($column_data[1]))
				{
					$default_val = "'" . $column_data[1] . "'";
					$return_array['null'] = 'NOT NULL';
					$sql .= 'NOT NULL ';
				}

				$return_array['default'] = $default_val;

				$sql .= "DEFAULT {$default_val}";

				// Unsigned? Then add a CHECK contraint
				if (in_array($orig_column_type, $this->unsigned_types))
				{
					$return_array['constraint'] = "CHECK ({$column_name} >= 0)";
					$sql .= " CHECK ({$column_name} >= 0)";
				}
			break;

			case 'sqlite':
				if (isset($column_data[2]) && $column_data[2] == 'auto_increment')
				{
					$sql .= ' INTEGER PRIMARY KEY';
				}
				else
				{
					$sql .= ' ' . $column_type;
				}

				$sql .= ' NOT NULL ';
				$sql .= (!is_null($column_data[1])) ? "DEFAULT '{$column_data[1]}'" : '';
			break;
		}

		$return_array['column_type_sql'] = $sql;

		return $return_array;
	}

	/**
	* Add new column
	*/
	function sql_column_add($table_name, $column_name, $column_data)
	{
		$column_data = $this->sql_prepare_column_data($table_name, $column_name, $column_data);
		$statements = array();

		switch ($this->sql_layer)
		{
			case 'firebird':
				$statements[] = 'ALTER TABLE "' . $table_name . '" ADD "' . $column_name . '" ' . $column_data['column_type_sql'];
			break;

			case 'mssql':
				$statements[] = 'ALTER TABLE [' . $table_name . '] ADD [' . $column_name . '] ' . $column_data['column_type_sql_default'];
			break;

			case 'mysql_40':
			case 'mysql_41':
				$statements[] = 'ALTER TABLE `' . $table_name . '` ADD COLUMN `' . $column_name . '` ' . $column_data['column_type_sql'];
			break;

			case 'oracle':
				$statements[] = 'ALTER TABLE ' . $table_name . ' ADD ' . $column_name . ' ' . $column_data['column_type_sql'];
			break;

			case 'postgres':
				$statements[] = 'ALTER TABLE ' . $table_name . ' ADD COLUMN "' . $column_name . '" ' . $column_data['column_type_sql'];
			break;

			case 'sqlite':
				if (version_compare(sqlite_libversion(), '3.0') == -1)
				{
					$sql = "SELECT sql
						FROM sqlite_master
						WHERE type = 'table'
							AND name = '{$table_name}'
						ORDER BY type DESC, name;";
					$result = $this->db->sql_query($sql);

					if (!$result)
					{
						break;
					}

					$row = $this->db->sql_fetchrow($result);
					$this->db->sql_freeresult($result);

					$statements[] = 'begin';

					// Create a backup table and populate it, destroy the existing one
					$statements[] = preg_replace('#CREATE\s+TABLE\s+"?' . $table_name . '"?#i', 'CREATE TEMPORARY TABLE ' . $table_name . '_temp', $row['sql']);
					$statements[] = 'INSERT INTO ' . $table_name . '_temp SELECT * FROM ' . $table_name;
					$statements[] = 'DROP TABLE ' . $table_name;

					preg_match('#\((.*)\)#s', $row['sql'], $matches);

					$new_table_cols = trim($matches[1]);
					$old_table_cols = preg_split('/,(?![\s\w]+\))/m', $new_table_cols);
					$column_list = array();

					foreach ($old_table_cols as $declaration)
					{
						$entities = preg_split('#\s+#', trim($declaration));
						if ($entities[0] == 'PRIMARY')
						{
							continue;
						}
						$column_list[] = $entities[0];
					}

					$columns = implode(',', $column_list);

					$new_table_cols = $column_name . ' ' . $column_data['column_type_sql'] . ',' . $new_table_cols;

					// create a new table and fill it up. destroy the temp one
					$statements[] = 'CREATE TABLE ' . $table_name . ' (' . $new_table_cols . ');';
					$statements[] = 'INSERT INTO ' . $table_name . ' (' . $columns . ') SELECT ' . $columns . ' FROM ' . $table_name . '_temp;';
					$statements[] = 'DROP TABLE ' . $table_name . '_temp';

					$statements[] = 'commit';
				}
				else
				{
					$statements[] = 'ALTER TABLE ' . $table_name . ' ADD ' . $column_name . ' [' . $column_data['column_type_sql'] . ']';
				}
			break;
		}

		return $this->_sql_run_sql($statements);
	}

	/**
	* Drop column
	*/
	function sql_column_remove($table_name, $column_name)
	{
		$statements = array();

		switch ($this->sql_layer)
		{
			case 'firebird':
				$statements[] = 'ALTER TABLE "' . $table_name . '" DROP "' . $column_name . '"';
			break;

			case 'mssql':
				$statements[] = 'ALTER TABLE [' . $table_name . '] DROP COLUMN [' . $column_name . ']';
			break;

			case 'mysql_40':
			case 'mysql_41':
				$statements[] = 'ALTER TABLE `' . $table_name . '` DROP COLUMN `' . $column_name . '`';
			break;

			case 'oracle':
				$statements[] = 'ALTER TABLE ' . $table_name . ' DROP ' . $column_name;
			break;

			case 'postgres':
				$statements[] = 'ALTER TABLE ' . $table_name . ' DROP COLUMN "' . $column_name . '"';
			break;

			case 'sqlite':
				if (version_compare(sqlite_libversion(), '3.0') == -1)
				{
					$sql = "SELECT sql
						FROM sqlite_master
						WHERE type = 'table'
							AND name = '{$table_name}'
						ORDER BY type DESC, name;";
					$result = $this->db->sql_query($sql);

					if (!$result)
					{
						break;
					}

					$row = $this->db->sql_fetchrow($result);
					$this->db->sql_freeresult($result);

					$statements[] = 'begin';

					// Create a backup table and populate it, destroy the existing one
					$statements[] = preg_replace('#CREATE\s+TABLE\s+"?' . $table_name . '"?#i', 'CREATE TEMPORARY TABLE ' . $table_name . '_temp', $row['sql']);
					$statements[] = 'INSERT INTO ' . $table_name . '_temp SELECT * FROM ' . $table_name;
					$statements[] = 'DROP TABLE ' . $table_name;

					preg_match('#\((.*)\)#s', $row['sql'], $matches);

					$new_table_cols = trim($matches[1]);
					$old_table_cols = preg_split('/,(?![\s\w]+\))/m', $new_table_cols);
					$column_list = array();

					foreach ($old_table_cols as $declaration)
					{
						$entities = preg_split('#\s+#', trim($declaration));
						if ($entities[0] == 'PRIMARY' || $entities[0] === $column_name)
						{
							continue;
						}
						$column_list[] = $entities[0];
					}

					$columns = implode(',', $column_list);

					$new_table_cols = $new_table_cols = preg_replace('/' . $column_name . '[^,]+(?:,|$)/m', '', $new_table_cols);

					// create a new table and fill it up. destroy the temp one
					$statements[] = 'CREATE TABLE ' . $table_name . ' (' . $new_table_cols . ');';
					$statements[] = 'INSERT INTO ' . $table_name . ' (' . $columns . ') SELECT ' . $columns . ' FROM ' . $table_name . '_temp;';
					$statements[] = 'DROP TABLE ' . $table_name . '_temp';

					$statements[] = 'commit';
				}
				else
				{
					$statements[] = 'ALTER TABLE ' . $table_name . ' DROP COLUMN ' . $column_name;
				}
			break;
		}

		return $this->_sql_run_sql($statements);
	}

	/**
	* Drop Index
	*/
	function sql_index_drop($table_name, $index_name)
	{
		$statements = array();

		switch ($this->sql_layer)
		{
			case 'mssql':
				$statements[] = 'DROP INDEX ' . $table_name . '.' . $index_name;
			break;

			case 'mysql_40':
			case 'mysql_41':
				$statements[] = 'DROP INDEX ' . $index_name . ' ON ' . $table_name;
			break;

			case 'firebird':
			case 'oracle':
			case 'postgres':
			case 'sqlite':
				$statements[] = 'DROP INDEX ' . $table_name . '_' . $index_name;
			break;
		}

		return $this->_sql_run_sql($statements);
	}

	/**
	* Add primary key
	*/
	function sql_create_primary_key($table_name, $column)
	{
		$statements = array();

		switch ($this->sql_layer)
		{
			case 'firebird':
			case 'postgres':
				$statements[] = 'ALTER TABLE ' . $table_name . ' ADD PRIMARY KEY (' . implode(', ', $column) . ')';
			break;

			case 'mssql':
				$sql = "ALTER TABLE [{$table_name}] WITH NOCHECK ADD ";
				$sql .= "CONSTRAINT [PK_{$table_name}] PRIMARY KEY  CLUSTERED (";
				$sql .= '[' . implode("],\n\t\t[", $column) . ']';
				$sql .= ') ON [PRIMARY]';

				$statements[] = $sql;
			break;

			case 'mysql_40':
			case 'mysql_41':
				$statements[] = 'ALTER TABLE ' . $table_name . ' ADD PRIMARY KEY (' . implode(', ', $column) . ')';
			break;

			case 'oracle':
				$statements[] = 'ALTER TABLE ' . $table_name . 'add CONSTRAINT pk_' . $table_name . ' PRIMARY KEY (' . implode(', ', $column) . ')';
			break;

			case 'sqlite':
				$sql = "SELECT sql
					FROM sqlite_master
					WHERE type = 'table'
						AND name = '{$table_name}'
					ORDER BY type DESC, name;";
				$result = $this->db->sql_query($sql);

				if (!$result)
				{
					break;
				}

				$row = $this->db->sql_fetchrow($result);
				$this->db->sql_freeresult($result);

				$statements[] = 'begin';

				// Create a backup table and populate it, destroy the existing one
				$statements[] = preg_replace('#CREATE\s+TABLE\s+"?' . $table_name . '"?#i', 'CREATE TEMPORARY TABLE ' . $table_name . '_temp', $row['sql']);
				$statements[] = 'INSERT INTO ' . $table_name . '_temp SELECT * FROM ' . $table_name;
				$statements[] = 'DROP TABLE ' . $table_name;

				preg_match('#\((.*)\)#s', $row['sql'], $matches);

				$new_table_cols = trim($matches[1]);
				$old_table_cols = preg_split('/,(?![\s\w]+\))/m', $new_table_cols);
				$column_list = array();

				foreach ($old_table_cols as $declaration)
				{
					$entities = preg_split('#\s+#', trim($declaration));
					if ($entities[0] == 'PRIMARY')
					{
						continue;
					}
					$column_list[] = $entities[0];
				}

				$columns = implode(',', $column_list);

				// create a new table and fill it up. destroy the temp one
				$statements[] = 'CREATE TABLE ' . $table_name . ' (' . $new_table_cols . ', PRIMARY KEY (' . implode(', ', $column) . '));';
				$statements[] = 'INSERT INTO ' . $table_name . ' (' . $columns . ') SELECT ' . $columns . ' FROM ' . $table_name . '_temp;';
				$statements[] = 'DROP TABLE ' . $table_name . '_temp';

				$statements[] = 'commit';
			break;
		}

		return $this->_sql_run_sql($statements);
	}

	/**
	* Add unique index
	*/
	function sql_create_unique_index($table_name, $index_name, $column)
	{
		$statements = array();

		switch ($this->sql_layer)
		{
			case 'firebird':
			case 'postgres':
			case 'oracle':
			case 'sqlite':
				$statements[] = 'CREATE UNIQUE INDEX ' . $table_name . '_' . $index_name . ' ON ' . $table_name . '(' . implode(', ', $column) . ')';
			break;

			case 'mysql_40':
			case 'mysql_41':
				$statements[] = 'CREATE UNIQUE INDEX ' . $index_name . ' ON ' . $table_name . '(' . implode(', ', $column) . ')';
			break;

			case 'mssql':
				$statements[] = 'CREATE UNIQUE INDEX ' . $index_name . ' ON ' . $table_name . '(' . implode(', ', $column) . ') ON [PRIMARY]';
			break;
		}

		return $this->_sql_run_sql($statements);
	}

	/**
	* Add index
	*/
	function sql_create_index($table_name, $index_name, $column)
	{
		$statements = array();

		switch ($this->sql_layer)
		{
			case 'firebird':
			case 'postgres':
			case 'oracle':
			case 'sqlite':
				$statements[] = 'CREATE INDEX ' . $table_name . '_' . $index_name . ' ON ' . $table_name . '(' . implode(', ', $column) . ')';
			break;

			case 'mysql_40':
			case 'mysql_41':
				$statements[] = 'CREATE INDEX ' . $index_name . ' ON ' . $table_name . '(' . implode(', ', $column) . ')';
			break;

			case 'mssql':
				$statements[] = 'CREATE INDEX ' . $index_name . ' ON ' . $table_name . '(' . implode(', ', $column) . ') ON [PRIMARY]';
			break;
		}

		return $this->_sql_run_sql($statements);
	}

	/**
	* List all of the indices that belong to a table,
	* does not count:
	* * UNIQUE indices
	* * PRIMARY keys
	*/
	function sql_list_index($table_name)
	{
		$index_array = array();

		if ($this->sql_layer == 'mssql')
		{
			$sql = "EXEC sp_statistics '$table_name'";
			$result = $this->db->sql_query($sql);
			while ($row = $this->db->sql_fetchrow($result))
			{
				if ($row['TYPE'] == 3)
				{
					$index_array[] = $row['INDEX_NAME'];
				}
			}
			$this->db->sql_freeresult($result);
		}
		else
		{
			switch ($this->sql_layer)
			{
				case 'firebird':
					$sql = "SELECT LOWER(RDB\$INDEX_NAME) as index_name
						FROM RDB\$INDICES
						WHERE RDB\$RELATION_NAME = " . strtoupper($table_name) . "
							AND RDB\$UNIQUE_FLAG IS NULL
							AND RDB\$FOREIGN_KEY IS NULL";
					$col = 'index_name';
				break;

				case 'postgres':
					$sql = "SELECT ic.relname as index_name
						FROM pg_class bc, pg_class ic, pg_index i
						WHERE (bc.oid = i.indrelid)
							AND (ic.oid = i.indexrelid)
							AND (bc.relname = '" . $table_name . "')
							AND (i.indisunique != 't')
							AND (i.indisprimary != 't')";
					$col = 'index_name';
				break;

				case 'mysql_40':
				case 'mysql_41':
					$sql = 'SHOW KEYS
						FROM ' . $table_name;
					$col = 'Key_name';
				break;

				case 'oracle':
					$sql = "SELECT index_name
						FROM user_indexes
						WHERE table_name = '" . $table_name . "'
							AND generated = 'N'";
				break;

				case 'sqlite':
					$sql = "PRAGMA index_info('" . $table_name . "');";
					$col = 'name';
				break;
			}

			$result = $this->db->sql_query($sql);
			while ($row = $this->db->sql_fetchrow($result))
			{
				if (($this->sql_layer == 'mysql_40' || $this->sql_layer == 'mysql_41') && !$row['Non_unique'])
				{
					continue;
				}

				switch ($this->sql_layer)
				{
					case 'firebird':
					case 'oracle':
					case 'postgres':
					case 'sqlite':
						$row[$col] = substr($row[$col], strlen($table_name) + 1);
					break;
				}

				$index_array[] = $row[$col];
			}
			$this->db->sql_freeresult($result);
		}

		return array_map('strtolower', $index_array);
	}

	/**
	* Change column type (not name!)
	*/
	function sql_column_change($table_name, $column_name, $column_data)
	{
		$column_data = $this->sql_prepare_column_data($table_name, $column_name, $column_data);
		$statements = array();

		switch ($this->sql_layer)
		{
			case 'firebird':
				// Change type...
				$statements[] = 'ALTER TABLE "' . $table_name . '" ALTER COLUMN "' . $column_name . '" TYPE ' . ' ' . $column_data['column_type_sql'];
			break;

			case 'mssql':
				$statements[] = 'ALTER TABLE [' . $table_name . '] ALTER COLUMN [' . $column_name . '] ' . $column_data['column_type_sql'];
			break;

			case 'mysql_40':
			case 'mysql_41':
				$statements[] = 'ALTER TABLE `' . $table_name . '` CHANGE `' . $column_name . '` `' . $column_name . '` ' . $column_data['column_type_sql'];
			break;

			case 'oracle':
				$statements[] = 'ALTER TABLE ' . $table_name . ' MODIFY ' . $column_name . ' ' . $column_data['column_type_sql'];
			break;

			case 'postgres':
				$sql = 'ALTER TABLE ' . $table_name . ' ';

				$sql_array = array();
				$sql_array[] = 'ALTER COLUMN ' . $column_name . ' TYPE ' . $column_data['column_type'];

				if (isset($column_data['null']))
				{
					if ($column_data['null'] == 'NOT NULL')
					{
						$sql_array[] = 'ALTER COLUMN ' . $column_name . ' SET NOT NULL';
					}
					else if ($column_data['null'] == 'NULL')
					{
						$sql_array[] = 'ALTER COLUMN ' . $column_name . ' DROP NOT NULL';
					}
				}

				if (isset($column_data['default']))
				{
					$sql_array[] = 'ALTER COLUMN ' . $column_name . ' SET DEFAULT ' . $column_data['default'];
				}

				// we don't want to double up on constraints if we change different number data types
				if (isset($column_data['constraint']))
				{
					$constraint_sql = "SELECT consrc as constraint_data
								FROM pg_constraint, pg_class bc
								WHERE conrelid = bc.oid
									AND bc.relname = '{$table_name}'
									AND NOT EXISTS (
										SELECT *
											FROM pg_constraint as c, pg_inherits as i
											WHERE i.inhrelid = pg_constraint.conrelid
												AND c.conname = pg_constraint.conname
												AND c.consrc = pg_constraint.consrc
												AND c.conrelid = i.inhparent
									)";

					$constraint_exists = false;

					$result = $this->db->sql_query($constraint_sql);
					while ($row = $this->db->sql_fetchrow($result))
					{
						if (trim($row['constraint_data']) == trim($column_data['constraint']))
						{
							$constraint_exists = true;
							break;
						}
					}
					$this->db->sql_freeresult($result);

					if (!$constraint_exists)
					{
						$sql_array[] = 'ADD ' . $column_data['constraint'];
					}
				}

				$sql .= implode(', ', $sql_array);

				$statements[] = $sql;
			break;

			case 'sqlite':

				$sql = "SELECT sql
					FROM sqlite_master
					WHERE type = 'table'
						AND name = '{$table_name}'
					ORDER BY type DESC, name;";
				$result = $this->db->sql_query($sql);

				if (!$result)
				{
					break;
				}

				$row = $this->db->sql_fetchrow($result);
				$this->db->sql_freeresult($result);

				$statements[] = 'begin';

				// Create a temp table and populate it, destroy the existing one
				$statements[] = preg_replace('#CREATE\s+TABLE\s+"?' . $table_name . '"?#i', 'CREATE TEMPORARY TABLE ' . $table_name . '_temp', $row['sql']);
				$statements[] = 'INSERT INTO ' . $table_name . '_temp SELECT * FROM ' . $table_name;
				$statements[] = 'DROP TABLE ' . $table_name;

				preg_match('#\((.*)\)#s', $row['sql'], $matches);

				$new_table_cols = trim($matches[1]);
				$old_table_cols = preg_split('/,(?![\s\w]+\))/m', $new_table_cols);
				$column_list = array();

				foreach ($old_table_cols as $key => $declaration)
				{
					$entities = preg_split('#\s+#', trim($declaration));
					$column_list[] = $entities[0];
					if ($entities[0] == $column_name)
					{
						$old_table_cols[$key] = $column_name . ' ' . $column_data['column_type_sql'];
					}
				}

				$columns = implode(',', $column_list);

				// create a new table and fill it up. destroy the temp one
				$statements[] = 'CREATE TABLE ' . $table_name . ' (' . implode(',', $old_table_cols) . ');';
				$statements[] = 'INSERT INTO ' . $table_name . ' (' . $columns . ') SELECT ' . $columns . ' FROM ' . $table_name . '_temp;';
				$statements[] = 'DROP TABLE ' . $table_name . '_temp';

				$statements[] = 'commit';

			break;
		}

		return $this->_sql_run_sql($statements);
	}
}

?>