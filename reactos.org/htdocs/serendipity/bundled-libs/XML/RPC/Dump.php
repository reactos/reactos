<?php

/* vim: set expandtab tabstop=4 shiftwidth=4 softtabstop=4: */

/**
 * Function and class to dump XML_RPC_Value objects in a nice way
 *
 * Should be helpful as a normal var_dump(..) displays all internals which
 * doesn't really give you an overview due to too much information.
 *
 * @category   Web Services
 * @package    XML_RPC
 * @author     Christian Weiske <cweiske@php.net>
 * @version    CVS: $Id: Dump.php,v 1.7 2005/01/24 03:47:55 danielc Exp $
 * @link       http://pear.php.net/package/XML_RPC
 */


/**
 * Pull in the XML_RPC class
 */
require_once 'XML/RPC.php';


/**
 * Generates the dump of the XML_RPC_Value and echoes it
 *
 * @param object $value  the XML_RPC_Value object to dump
 *
 * @return void
 */
function XML_RPC_Dump($value)
{
    $dumper = new XML_RPC_Dump();
    echo $dumper->generateDump($value);
}


/**
 * Class which generates a dump of a XML_RPC_Value object
 *
 * @category   Web Services
 * @package    XML_RPC
 * @author     Christian Weiske <cweiske@php.net>
 * @version    Release: 1.4.0
 * @link       http://pear.php.net/package/XML_RPC
 */
class XML_RPC_Dump
{
    /**
     * The indentation array cache
     * @var array
     */
    var $arIndent      = array();

    /**
     * The spaces used for indenting the XML
     * @var string
     */
    var $strBaseIndent = '    ';

    /**
     * Returns the dump in XML format without printing it out
     *
     * @param object $value   the XML_RPC_Value object to dump
     * @param int    $nLevel  the level of indentation
     *
     * @return string  the dump
     */
    function generateDump($value, $nLevel = 0)
    {
        if (!is_object($value) && get_class($value) != 'xml_rpc_value') {
            require_once 'PEAR.php';
            PEAR::raiseError('Tried to dump non-XML_RPC_Value variable' . "\r\n",
                             0, PEAR_ERROR_PRINT);
            if (is_object($value)) {
                $strType = get_class($value);
            } else {
                $strType = gettype($value);
            }
            return $this->getIndent($nLevel) . 'NOT A XML_RPC_Value: '
                   . $strType . "\r\n";
        }

        switch ($value->kindOf()) {
        case 'struct':
            $ret = $this->genStruct($value, $nLevel);
            break;
        case 'array':
            $ret = $this->genArray($value, $nLevel);
            break;
        case 'scalar':
            $ret = $this->genScalar($value->scalarval(), $nLevel);
            break;
        default:
            require_once 'PEAR.php';
            PEAR::raiseError('Illegal type "' . $value->kindOf()
                             . '" in XML_RPC_Value' . "\r\n", 0,
                             PEAR_ERROR_PRINT);
        }

        return $ret;
    }

    /**
     * Returns the scalar value dump
     *
     * @param object $value   the scalar XML_RPC_Value object to dump
     * @param int    $nLevel  the level of indentation
     *
     * @return string  Dumped version of the scalar value
     */
    function genScalar($value, $nLevel)
    {
        if (gettype($value) == 'object') {
            $strClass = ' ' . get_class($value);
        } else {
            $strClass = '';
        }
        return $this->getIndent($nLevel) . gettype($value) . $strClass
               . ' ' . $value . "\r\n";
    }

    /**
     * Returns the dump of a struct
     *
     * @param object $value   the struct XML_RPC_Value object to dump
     * @param int    $nLevel  the level of indentation
     *
     * @return string  Dumped version of the scalar value
     */
    function genStruct($value, $nLevel)
    {
        $value->structreset();
        $strOutput = $this->getIndent($nLevel) . 'struct' . "\r\n";
        while (list($key, $keyval) = $value->structeach()) {
            $strOutput .= $this->getIndent($nLevel + 1) . $key . "\r\n";
            $strOutput .= $this->generateDump($keyval, $nLevel + 2);
        }
        return $strOutput;
    }

    /**
     * Returns the dump of an array
     *
     * @param object $value   the array XML_RPC_Value object to dump
     * @param int    $nLevel  the level of indentation
     *
     * @return string  Dumped version of the scalar value
     */
    function genArray($value, $nLevel)
    {
        $nSize     = $value->arraysize();
        $strOutput = $this->getIndent($nLevel) . 'array' . "\r\n";
        for($nA = 0; $nA < $nSize; $nA++) {
            $strOutput .= $this->getIndent($nLevel + 1) . $nA . "\r\n";
            $strOutput .= $this->generateDump($value->arraymem($nA),
                                              $nLevel + 2);
        }
        return $strOutput;
    }

    /**
     * Returns the indent for a specific level and caches it for faster use
     *
     * @param int $nLevel  the level
     *
     * @return string  the indented string
     */
    function getIndent($nLevel)
    {
        if (!isset($this->arIndent[$nLevel])) {
            $this->arIndent[$nLevel] = str_repeat($this->strBaseIndent, $nLevel);
        }
        return $this->arIndent[$nLevel];
    }
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * c-hanging-comment-ender-p: nil
 * End:
 */

?>
