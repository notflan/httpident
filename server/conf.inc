<?php
require_once "httpi-tools.inc";

#Token config
hiConfSet("tokExpiresIn", 30); //Seconds or null for never

#Database config
hiConfSet("dbHost", "127.0.0.1");
hiConfSet("dbUser", "httpi");
hiConfSet("dbPasswd", "password");
hiConfSet("dbName", "httpi");

#Environment config
hiConfSet("cgiLocation", __DIR__. "/cgi-bin");
hiConfSet("pythonExecutablePath", "python");
hiConfSet("tempPath", "/tmp");
hiConfSet("gpgHome", "/var/lib/nginx/.gnupg");

?>
