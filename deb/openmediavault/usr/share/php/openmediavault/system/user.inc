<?php
/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2016 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
namespace OMV\System;

/**
 * This class provides a simple interface to handle a system user.
 * @ingroup api
 */
class User {
	protected $name = null;
	protected $uid = null;
	protected $gid = null;
	protected $password = null;
	protected $gecos = null;
	protected $dir = null;
	protected $shell = null;
	protected $lastchanged = null;
	protected $minimum = null;
	protected $maximum = null;
	protected $warn = null;
	protected $inactive = null;
	protected $expire = null;
	protected $reserved = null;
	protected $groups = null;

	/**
	 * Constructor
	 * @param id The name or UID of the system user.
	 */
	public function __construct($id) {
		if (is_int($id))
			$this->uid = $id;
		else
			$this->name = $id;
	}

	/**
	 * Get the system group data.
	 * @private
	 * @return None.
	 * @throw \OMV\Exception
	 * @throw \OMV\ExecException
	 */
	private function getData($type) {
		switch ($type) {
		case "shadow":
			if (!is_null($this->password))
				return;

			// http://www.cyberciti.biz/faq/understanding-etcshadow-file
			$cmdArgs = [];
			//$cmdArgs[] = "shadow";
			//$cmdArgs[] = escapeshellarg($this->getName());
			//$cmd = new \OMV\System\Process("getent", $cmdArgs);
			//$cmd->setRedirect2to1();
			//$cmd->execute($output);
			$output = [];
			$output[] = $this->getName().":*:16317:0:99999:7:::";
			$output = explode(":", $output[0]);
			$this->password = $output[1];
			$this->lastchanged = $output[2];
			$this->minimum = $output[3];
			$this->maximum = $output[4];
			$this->warn = $output[5];
			$this->inactive = $output[6];
			$this->expire = $output[7];
			$this->reserved = $output[8];
			break;
		case "passwd":
			if (!is_null($this->name) && !is_null($this->uid))
				return;
			// Query user information.
			$userInfo = FALSE;
			if (!is_null($this->uid))
				$userInfo = new_posix_getpwuid($this->uid);
			else
				$userInfo = new_posix_getpwnam($this->name);
			if (FALSE === $userInfo) {
				throw new \OMV\Exception(
				  "Failed to get information about the user '%s': %s",
				  !is_null($this->uid) ? strval($this->uid) : $this->name,
				  posix_strerror(posix_errno()));
			}
			$this->name = $userInfo['name'];
			$this->uid = $userInfo['uid'];
			$this->gid = $userInfo['gid'];
			$this->gecos = $userInfo['gecos'];
			$this->dir = $userInfo['dir'];
			$this->shell = $userInfo['shell'];
			break;
		case "groups":
			if (!is_null($this->groups))
				return;
			// Get the group IDs the user is in as white-space separated
			// numbers. Thus it is possible to correctly process group
			// names that contain blanks (e.g. imported via LDAP or AD).
			$cmdArgs = [];
			$cmdArgs[] = "-G";
			$cmdArgs[] = escapeshellarg($this->getName());
			$cmd = new \OMV\System\Process("id", $cmdArgs);
			$cmd->setRedirect2to1();
			$cmd->execute($output);
			$gids = explode(" ", $output[0]);
			// Resolve the group names.
			$this->groups = [];
			foreach ($gids as $gidk => $gidv) {
				$groupInfo = new_posix_getgrgid($gidv);
				$this->groups[] = $groupInfo['name'];
			}
			break;
		}
	}

	/**
	 * Check whether the system user exists.
	 * @return TRUE if the system user exists, otherwise FALSE.
	 */
	public function exists() {
		try {
			$this->getData("passwd");
		} catch(\Exception $e) {
			return FALSE;
		}
		return !is_null($this->name) && !is_null($this->uid);
	}

	/**
	 * Assert that the system user exists.
	 * @throw \OMV\AssertException
	 */
	public function assertExists() {
		if (FALSE === $this->exists()) {
			throw new \OMV\AssertException("The user '%s' does not exist.",
			  !is_null($this->uid) ? strval($this->uid) : $this->name);
		}
	}

	/**
	 * Assert that the system user not exists.
	 * @throw \OMV\AssertException
	 */
	public function assertNotExists() {
		if (TRUE === $this->exists()) {
			throw new \OMV\AssertException("The user '%s' already exists.",
			  !is_null($this->uid) ? strval($this->uid) : $this->name);
		}
	}

	/**
	 * Get the user name.
	 * @return The user name.
	 */
	public function getName() {
		$this->getData("passwd");
		return $this->name;
	}

	/**
	 * Get the user ID.
	 * @return The user ID, otherwise FALSE.
	 */
	public function getUid() {
		$this->getData("passwd");
		return $this->uid;
	}

	/**
	 * Get the users group ID.
	 * @return The user group ID, otherwise FALSE.
	 */
	public function getGid() {
		$this->getData("passwd");
		return $this->gid;
	}

	/**
	 * Get the comment.
	 * @return The comment, otherwise FALSE.
	 */
	public function getGecos() {
		$this->getData("passwd");
		return $this->gecos;
	}

	/**
	 * Get the home directory.
	 * @return The home directory, otherwise FALSE.
	 */
	public function getHomeDirectory() {
		$this->getData("passwd");
		return $this->dir;
	}

	/**
	 * Get the shell.
	 * @return The shell, otherwise FALSE.
	 */
	public function getShell() {
		$this->getData("passwd");
		return $this->shell;
	}

	/**
	 * Get the encrypted password.
	 * @return The encrypted password, otherwise FALSE.
	 */
	public function getPassword() {
		$this->getData("shadow");
		return $this->password;
	}

	public function getLastChanged() {
		$this->getData("shadow");
		return $this->lastchanged;
	}

	public function getMinimum() {
		$this->getData("shadow");
		return $this->minimum;
	}

	public function getMaximum() {
		$this->getData("shadow");
		return $this->maximum;
	}

	public function getWarn() {
		$this->getData("shadow");
		return $this->warn;
	}

	public function getInactive() {
		$this->getData("shadow");
		return $this->inactive;
	}

	public function getExpire() {
		$this->getData("shadow");
		return $this->expire;
	}

	public function getReserved() {
		$this->getData("shadow");
		return $this->reserved;
	}

	/**
	 * Get the groups the user is in.
	 * @return An array of groups the user is in, otherwise FALSE.
	 */
	public function getGroups() {
		$this->getData("groups");
		return $this->groups;
	}

	/**
	 * Get the user quotas.
	 * @return An array containing the quotas.
	 * @throw \OMV\ExecException
	 */
	public function getQuotas() {
		$cmdArgs = [];
		$cmdArgs[] = "-u";
		$cmdArgs[] = escapeshellarg($this->getName());
		$cmd = new \OMV\System\Process("edquota", $cmdArgs);
		$cmd->setEnv("EDITOR", "cat");
		$cmd->setRedirect2to1();
		$cmd->execute($output);
		// Parse command output:
		// Filesystem                   blocks       soft       hard     inodes     soft     hard
		// /dev/sdb1                     10188          0      12288          4        0        0
		// /dev/sdc1                         0          0      45056          0        0        0
		$result = [];
		foreach ($output as $outputk => $outputv) {
			if (preg_match("/^\s+(\S+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+".
			  "(\d+)\s+(\d+)$/i", $outputv, $matches)) {
				$result[$matches[1]] = [
					"devicefile" => $matches[1],
					"blocks" => $matches[2],
					"bsoft" => $matches[3],
					"bhard" => $matches[4],
					"inodes" => $matches[5],
					"isoft" => $matches[6],
					"ihard" => $matches[7]
				];
			}
		}
		return $result;
	}

	/**
	 * Authenticate user via PAM. Account expiration and access hours are
	 * checked, too. Note, the calling process must have the privilege to
	 * read the user shadow file.
	 * @param password The password.
	 * @return TRUE if the authentication was successful, otherwise FALSE.
	 */
	public function authenticate($password) {
		if (FALSE === pam_auth($this->getName(), $password))
			return FALSE;
		return TRUE;
	}

	/**
	 * Check if the given user is a system account.
	 * @return TRUE if the user is a system account, otherwise FALSE.
	 */
	public function isSystemAccount() {
		if (FALSE === ($uid = $this->getUid()))
			return FALSE;
		// Get shadow password suite configuration.
		$ld = \OMV\System\System::getLoginDefs();
		// Get the min/max values of the non-system account ID sequence.
		$min = intval(array_value($ld, "UID_MIN", 1000));
		$max = intval(array_value($ld, "UID_MAX", 60000));
		// Check if the given account ID is within the sequence.
		return !in_range($uid, $min, $max);
	}

	/**
	 * Enumerate user names.
	 * @return An array of user names.
	 * @throw \OMV\ExecException
	 */
	public static function getUsers() {
		$cmd = new \OMV\System\Process("getent", "passwd |grep -v '^al\|^a0'");
		$cmd->setRedirect2to1();
		$cmd->execute($output);
		// Parse command output:
		// proftpd:x:109:65534::/var/run/proftpd:/bin/false
		// ftp:x:110:65534::/home/ftp:/bin/false
		// openmediavault:x:999:999::/home/openmediavault:/bin/sh
		// admin:x:998:100:WebGUI administrator:/home/admin:/usr/sbin/nologin
		// nut:x:111:114::/var/lib/nut:/bin/false
		// test:x:1001:100:sdfds:/home/test:/bin/dash
		$list = [];
		foreach ($output as $outputv) {
			$data = explode(":", $outputv);
			if (TRUE === empty($data))
				continue;
			$list[] = $data[0]; // User name
		}
		return $list;
	}
}


function new_posix_parser($txt){
	if(empty($txt)) {
		return FALSE;
	}

	$data = explode(":", $txt);

	$user_info=array("name"   => $data[0],
                         "passwd" => $data[1],
			 "uid"    => $data[2],
			 "gid"    => $data[3],
			 "gecos"  => $data[4],
			 "dir"	  => $data[5],
			 "shell"  => $data[6]);
	return $user_info;
}

function new_posix_getpwnam($username){
	$cmdout = shell_exec( "getent passwd ".$username);
	# "lgarcia:x:1000:1000:Luis,,,:/home/lgarcia:/bin/bash";
	return new_posix_parser($cmdout);	
}

function new_posix_getpwuid($userid){
	$cmdout = shell_exec( "getent passwd |grep '^\([^:]\+:\)\{2\}".strval($userid).":'");
	# "lgarcia:x:1000:1000:Luis,,,:/home/lgarcia:/bin/bash";
	return new_posix_parser($cmdout);	
}

function new_posix_getgrgid($groupid){
	$cmdout = shell_exec( "getent group |grep '^\([^:]\+:\)\{2\}".strval($groupid).":'");
	#adm:x:4:syslog,lgarcia
	if(empty($cmdout)) {
		return FALSE;
	}

	$data = explode(":", $cmdout);

	$group_info=array("name"   => $data[0],
                         "passwd"  => $data[1],
			 "gid"     => $data[2],
			 "members" => explode(",", $data[3]));
	return $group_info;
	
	
}

