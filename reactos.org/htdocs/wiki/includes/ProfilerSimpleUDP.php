<?php
/**
 * @file
 * @ingroup Profiler
 */

require_once(dirname(__FILE__).'/ProfilerSimple.php');

/**
 * ProfilerSimpleUDP class, that sends out messages for 'udpprofile' daemon
 * (the one from mediawiki/trunk/udpprofile SVN )
 * @ingroup Profiler
 */
class ProfilerSimpleUDP extends ProfilerSimple {
	function getFunctionReport() {
		global $wgUDPProfilerHost, $wgUDPProfilerPort;

		if ( $this->mCollated['-total']['real'] < $this->mMinimumTime ) {
			# Less than minimum, ignore
			return;
		}

		$sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
		$plength=0;
		$packet="";
		foreach ($this->mCollated as $entry=>$pfdata) {
			$pfline=sprintf ("%s %s %d %f %f %f %f %s\n", $this->getProfileID(),"-",$pfdata['count'],
				$pfdata['cpu'],$pfdata['cpu_sq'],$pfdata['real'],$pfdata['real_sq'],$entry);
			$length=strlen($pfline);
			/* printf("<!-- $pfline -->"); */
			if ($length+$plength>1400) {
				socket_sendto($sock,$packet,$plength,0,$wgUDPProfilerHost,$wgUDPProfilerPort);
				$packet="";
				$plength=0;
			}
			$packet.=$pfline;
			$plength+=$length;
		}
		socket_sendto($sock,$packet,$plength,0x100,$wgUDPProfilerHost,$wgUDPProfilerPort);
	}
}
