<?php
/**
*
* @package mcp
* @version $Id: mcp_reports.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
* @package module_install
*/
class mcp_reports_info
{
	function module()
	{
		return array(
			'filename'	=> 'mcp_reports',
			'title'		=> 'MCP_REPORTS',
			'version'	=> '1.0.0',
			'modes'		=> array(
				'reports'			=> array('title' => 'MCP_REPORTS_OPEN', 'auth' => 'aclf_m_report', 'cat' => array('MCP_REPORTS')),
				'reports_closed'	=> array('title' => 'MCP_REPORTS_CLOSED', 'auth' => 'aclf_m_report', 'cat' => array('MCP_REPORTS')),
				'report_details'	=> array('title' => 'MCP_REPORT_DETAILS', 'auth' => 'acl_m_report,$id || (!$id && aclf_m_report)', 'cat' => array('MCP_REPORTS')),
			),
		);
	}

	function install()
	{
	}

	function uninstall()
	{
	}
}

?>