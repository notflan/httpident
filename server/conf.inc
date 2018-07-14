<?php
require_once "httpi-tools.inc";

hiConfSet("dbHost", "127.0.0.1");
hiConfSet("dbUser", "httpi");
hiConfSet("dbPasswd", "password");
hiConfSet("dbName", "httpi");

hiConfSet("cgiLocation", __DIR__. "/cgi-bin");

hiConfSet("pythonExecutablePath", "python");

?>
