<?php
require_once "httpi-tools.inc";
require_once "conf.inc";

function httpi_create_token()
{
	return hash("sha256",microtime()."".random_bytes(100));
}

function httpi_connect_to($host, $user, $password, $database)
{
	$con = new mysqli($host, $user, $password,$database);
	if($con->connect_errno) return null;
	return $con;
}

function httpi_connect()
{
	return httpi_connect_to(hiConfGet("dbHost"), hiConfGet("dbUser"), hiConfGet("dbPasswd"), hiConfGet("dbName"));
}

function httpi_setup_database($con)
{
	$ok = $con->query("CREATE TABLE IF NOT EXISTS tokens (
		id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
		token VARCHAR(64) NOT NULL,
		url VARCHAR(128) NOT NULL,
		who INT UNSIGNED NOT NULL,
		timestamp TIMESTAMP NOT NULL,
		expires_at DATETIME DEFAULT NULL,
		expired BOOL DEFAULT FALSE NOT NULL,
		expire_reason INT DEFAULT 0 NOT NULL
	) ENGINE=InnoDB");
	if(!$ok) return false; else
	$ok = $con->query("CREATE TABLE IF NOT EXISTS authlog (
		id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
		token_id INT NOT NULL,
		authed_by VARCHAR(8) DEFAULT NULL,
		index by_token (token_id),
		CONSTRAINT c_token
		FOREIGN KEY fk_token(token_id)
		REFERENCES tokens(id)
		ON UPDATE CASCADE
		ON DELETE RESTRICT
	) ENGINE=InnoDB");
	return $ok;
}

?>
