<?php
/**
 * Aliases for functions in the Xml module
 * Look at the Xml class (Xml.php) for the implementations.
 */
function wfElement( $element, $attribs = null, $contents = '') {
	return Xml::element( $element, $attribs, $contents );
}
function wfElementClean( $element, $attribs = array(), $contents = '') {
	return Xml::elementClean( $element, $attribs, $contents );
}
function wfOpenElement( $element, $attribs = null ) {
	return Xml::openElement( $element, $attribs );
}
function wfCloseElement( $element ) {
	return "</$element>";
}
function HTMLnamespaceselector($selected = '', $allnamespaces = null, $includehidden=false) {
	return Xml::namespaceSelector( $selected, $allnamespaces, $includehidden );
}
function wfSpan( $text, $class, $attribs=array() ) {
	return Xml::span( $text, $class, $attribs );
}
function wfInput( $name, $size=false, $value=false, $attribs=array() ) {
	return Xml::input( $name, $size, $value, $attribs );
}
function wfAttrib( $name, $present = true ) {
	return Xml::attrib( $name, $present );
}
function wfCheck( $name, $checked=false, $attribs=array() ) {
	return Xml::check( $name, $checked, $attribs );
}
function wfRadio( $name, $value, $checked=false, $attribs=array() ) {
	return Xml::radio( $name, $value, $checked, $attribs );
}
function wfLabel( $label, $id ) {
	return Xml::label( $label, $id );
}
function wfInputLabel( $label, $name, $id, $size=false, $value=false, $attribs=array() ) {
	return Xml::inputLabel( $label, $name, $id, $size, $value, $attribs );
}
function wfCheckLabel( $label, $name, $id, $checked=false, $attribs=array() ) {
	return Xml::checkLabel( $label, $name, $id, $checked, $attribs );
}
function wfRadioLabel( $label, $name, $value, $id, $checked=false, $attribs=array() ) {
	return Xml::radioLabel( $label, $name, $value, $id, $checked, $attribs );
}
function wfSubmitButton( $value, $attribs=array() ) {
	return Xml::submitButton( $value, $attribs );
}
function wfHidden( $name, $value, $attribs=array() ) {
	return Xml::hidden( $name, $value, $attribs );
}
function wfEscapeJsString( $string ) {
	return Xml::escapeJsString( $string );
}
function wfIsWellFormedXml( $text ) {
	return Xml::isWellFormed( $text );
}
function wfIsWellFormedXmlFragment( $text ) {
	return Xml::isWellFormedXmlFragment( $text );
}

function wfBuildForm( $fields, $submitLabel ) {
	return Xml::buildForm( $fields, $submitLabel );
}
