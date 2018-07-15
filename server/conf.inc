<?php
require_once "httpi-tools.inc";
require_once "logging.inc";

#Misc config
hiConfSet("m403Message", "403 Forbidden");

hiConfSet("mEnableAccessLog", true);
hiConfSet("mLoggingLevel", HTTPI_AL_LOW);

hiConfSet("mFancyTtyOutput", true);

#Token config
hiConfSet("tokExpiresIn", 30); //Seconds or null for never

#Database config
hiConfSet("dbHost", "127.0.0.1");
hiConfSet("dbUser", "httpi");
hiConfSet("dbPasswd", "password");
hiConfSet("dbName", "httpi");
hiConfSet("dbEngine", "InnoDB");

#Environment config
hiConfSet("cgiLocation", __DIR__. "/cgi-bin");
hiConfSet("pythonExecutablePath", "python");
hiConfSet("tempPath", "/tmp");
hiConfSet("gpgHome", "/var/lib/nginx/.gnupg");

?>
