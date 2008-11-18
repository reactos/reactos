<?php

/**
 * Class to implement stub globals, which are globals that delay loading the
 * their associated module code by deferring initialisation until the first
 * method call.
 *
 * Note on unstub loops:
 *
 * Unstub loops (infinite recursion) sometimes occur when a constructor calls
 * another function, and the other function calls some method of the stub. The
 * best way to avoid this is to make constructors as lightweight as possible,
 * deferring any initialisation which depends on other modules. As a last
 * resort, you can use StubObject::isRealObject() to break the loop, but as a
 * general rule, the stub object mechanism should be transparent, and code
 * which refers to it should be kept to a minimum.
 */
class StubObject {
	var $mGlobal, $mClass, $mParams;

	/**
	 * Constructor.
	 *
	 * @param String $global name of the global variable.
	 * @param String $class name of the class of the real object.
	 * @param Array $param array of parameters to pass to contructor of the real
	 *                     object.
	 */
	function __construct( $global = null, $class = null, $params = array() ) {
		$this->mGlobal = $global;
		$this->mClass = $class;
		$this->mParams = $params;
	}

	/**
	 * Returns a bool value whetever $obj is a stub object. Can be used to break
	 * a infinite loop when unstubbing an object.
	 *
	 * @param Object $obj object to check.
	 * @return bool true if $obj is not an instance of StubObject class.
	 */
	static function isRealObject( $obj ) {
		return is_object( $obj ) && !($obj instanceof StubObject);
	}

	/**
	 * Function called if any function exists with that name in this object.
	 * It is used to unstub the object. Only used internally, PHP will call
	 * self::__call() function and that function will call this function.
	 * This function will also call the function with the same name in the real
	 * object.
	 *
	 * @param String $name name of the function called.
	 * @param Array $args array of arguments.
	 */
	function _call( $name, $args ) {
		$this->_unstub( $name, 5 );
		return call_user_func_array( array( $GLOBALS[$this->mGlobal], $name ), $args );
	}

	/**
	 * Create a new object to replace this stub object.
	 */
	function _newObject() {
		return wfCreateObject( $this->mClass, $this->mParams );
	}

	/**
	 * Function called by PHP if no function with that name exists in this
	 * object.
	 *
	 * @param String $name name of the function called
	 * @param Array $args array of arguments
	 */
	function __call( $name, $args ) {
		return $this->_call( $name, $args );
	}

	/**
	 * This function creates a new object of the real class and replace it in
	 * the global variable.
	 * This is public, for the convenience of external callers wishing to access
	 * properties, e.g. eval.php
	 *
	 * @param String $name name of the method called in this object.
	 * @param Integer $level level to go in the stact trace to get the function
	 *                       who called this function.
	 */
	function _unstub( $name = '_unstub', $level = 2 ) {
		static $recursionLevel = 0;
		if ( get_class( $GLOBALS[$this->mGlobal] ) != $this->mClass ) {
			$fname = __METHOD__.'-'.$this->mGlobal;
			wfProfileIn( $fname );
			$caller = wfGetCaller( $level );
			if ( ++$recursionLevel > 2 ) {
				throw new MWException( "Unstub loop detected on call of \${$this->mGlobal}->$name from $caller\n" );
			}
			wfDebug( "Unstubbing \${$this->mGlobal} on call of \${$this->mGlobal}->$name from $caller\n" );
			$GLOBALS[$this->mGlobal] = $this->_newObject();
			--$recursionLevel;
			wfProfileOut( $fname );
		}
	}
}

/**
 * Stub object for the content language of this wiki. This object have to be in
 * $wgContLang global.
 */
class StubContLang extends StubObject {

	function __construct() {
		parent::__construct( 'wgContLang' );
	}

	function __call( $name, $args ) {
		return $this->_call( $name, $args );
	}

	function _newObject() {
		global $wgContLanguageCode;
		$obj = Language::factory( $wgContLanguageCode );
		$obj->initEncoding();
		$obj->initContLang();
		return $obj;
	}
}

/**
 * Stub object for the user language. It depends of the user preferences and
 * "uselang" parameter that can be passed to index.php. This object have to be
 * in $wgLang global.
 */
class StubUserLang extends StubObject {

	function __construct() {
		parent::__construct( 'wgLang' );
	}

	function __call( $name, $args ) {
		return $this->_call( $name, $args );
	}

	function _newObject() {
		global $wgContLanguageCode, $wgRequest, $wgUser, $wgContLang;
		$code = $wgRequest->getVal( 'uselang', $wgUser->getOption( 'language' ) );

		// if variant is explicitely selected, use it instead the one from wgUser
		// see bug #7605
		if( $wgContLang->hasVariants() && in_array($code, $wgContLang->getVariants()) ){
			$variant = $wgContLang->getPreferredVariant();
			if( $variant != $wgContLanguageCode )
				$code = $variant;
		}

		# Validate $code
		if( empty( $code ) || !preg_match( '/^[a-z-]+$/', $code ) ) {
			wfDebug( "Invalid user language code\n" );
			$code = $wgContLanguageCode;
		}

		if( $code === $wgContLanguageCode ) {
			return $wgContLang;
		} else {
			$obj = Language::factory( $code );
			return $obj;
		}
	}
}

/**
 * Stub object for the user. The initialisation of the will depend of
 * $wgCommandLineMode. If it's true, it will be an anonymous user and if it's
 * false, the user will be loaded from credidentails provided by cookies. This
 * object have to be in $wgUser global.
 */
class StubUser extends StubObject {

	function __construct() {
		parent::__construct( 'wgUser' );
	}

	function __call( $name, $args ) {
		return $this->_call( $name, $args );
	}

	function _newObject() {
		global $wgCommandLineMode;
		if( $wgCommandLineMode ) {
			$user = new User;
		} else {
			$user = User::newFromSession();
		}
		return $user;
	}
}
