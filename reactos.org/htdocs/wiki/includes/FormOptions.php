<?php
/**
 * Helper class to keep track of options when mixing links and form elements.
 *
 * @author Niklas Laxström
 * @copyright Copyright © 2008, Niklas Laxström
 */

class FormOptions implements ArrayAccess {
	const AUTO = -1; //! Automatically detects simple data types
	const STRING = 0;
	const INT = 1;
	const BOOL = 2;
	const INTNULL = 3; //! Useful for namespace selector

	protected $options = array();

	# Setting up

	public function add( $name, $default, $type = self::AUTO ) {
		$option = array();
		$option['default'] = $default;
		$option['value'] = null;
		$option['consumed'] = false;

		if ( $type !== self::AUTO ) {
			$option['type'] = $type;
		} else {
			$option['type'] = self::guessType( $default );
		}

		$this->options[$name] = $option;
	}

	public function delete( $name ) {
		$this->validateName( $name, true );
		unset($this->options[$name]);
	}

	public static function guessType( $data ) {
		if ( is_bool($data) ) {
			return self::BOOL;
		} elseif( is_int($data) ) {
			return self::INT;
		} elseif( is_string($data) ) {
			return self::STRING;
		} else {
			throw new MWException( 'Unsupported datatype' );
		}
	}

	# Handling values

	public function validateName( $name, $strict = false ) {
		if ( !isset($this->options[$name]) ) {
			if ( $strict ) {
				throw new MWException( "Invalid option $name" );
			} else {
				return false;
			}
		}
		return true;
	}

	public function setValue( $name, $value, $force = false ) {
		$this->validateName( $name, true );
		if ( !$force && $value === $this->options[$name]['default'] ) {
			// null default values as unchanged
			$this->options[$name]['value'] = null;
		} else {
			$this->options[$name]['value'] = $value;
		}
	}

	public function getValue( $name ) {
		$this->validateName( $name, true );
		return $this->getValueReal( $this->options[$name] );
	}

	protected function getValueReal( $option ) {
		if ( $option['value'] !== null ) {
			return $option['value'];
		} else {
			return $option['default'];
		}
	}

	public function reset( $name ) {
		$this->validateName( $name, true );
		$this->options[$name]['value'] = null;
	}

	public function consumeValue( $name ) {
		$this->validateName( $name, true );
		$this->options[$name]['consumed'] = true;
		return $this->getValueReal( $this->options[$name] );
	}

	public function consumeValues( /*Array*/ $names ) {
		$out = array();
		foreach ( $names as $name ) {
			$this->validateName( $name, true );
			$this->options[$name]['consumed'] = true;
			$out[] = $this->getValueReal( $this->options[$name] );
		}
		return $out;
	}

	# Validating values

	public function validateIntBounds( $name, $min, $max ) {
		$this->validateName( $name, true );

		if ( $this->options[$name]['type'] !== self::INT )
			throw new MWException( "Option $name is not of type int" );

		$value = $this->getValueReal( $this->options[$name] );
		$value = max( $min, min( $max, $value ) );

		$this->setValue( $name, $value );
	}

	# Getting the data out for use

	public function getUnconsumedValues( $all = false ) {
		$values = array();
		foreach ( $this->options as $name => $data ) {
			if ( !$data['consumed'] ) {
				if ( $all || $data['value'] !== null ) {
					$values[$name] = $this->getValueReal( $data );
				}
			}
		}
		return $values;
	}

	public function getChangedValues() {
		$values = array();
		foreach ( $this->options as $name => $data ) {
			if ( $data['value'] !== null ) {
				$values[$name] = $data['value'];
			}
		}
		return $values;
	}

	public function getAllValues() {
		$values = array();
		foreach ( $this->options as $name => $data ) {
			$values[$name] = $this->getValueReal( $data );
		}
		return $values;
	}

	# Reading values

	public function fetchValuesFromRequest( WebRequest $r, $values = false ) {
		if ( !$values ) {
			$values = array_keys($this->options);
		}

		foreach ( $values as $name ) {
			$default = $this->options[$name]['default'];
			$type = $this->options[$name]['type'];

			switch( $type ) {
				case self::BOOL:
					$value = $r->getBool( $name, $default ); break;
				case self::INT:
					$value = $r->getInt( $name, $default ); break;
				case self::STRING:
					$value = $r->getText( $name, $default ); break;
				case self::INTNULL:
					$value = $r->getIntOrNull( $name ); break;
				default:
					throw new MWException( 'Unsupported datatype' );
			}

			if ( $value !== $default && $value !== null ) {
				$this->options[$name]['value'] = $value;
			}
		}
	}

	/* ArrayAccess methods */
	public function offsetExists( $name ) {
		return isset($this->options[$name]);
	}

	public function offsetGet( $name ) {
		return $this->getValue( $name );
	}

	public function offsetSet( $name, $value ) {
		return $this->setValue( $name, $value );
	}

	public function offsetUnset( $name ) {
		return $this->delete( $name );
	}

}
