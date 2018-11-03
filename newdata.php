<?php
/*
REST API pro iTRUBEC SigFox monitor

Formát Callbacku ze SigFoxu
http://itrubec.cz/monitor/newdata.php?key=xxxxxxxx&dev={device}&t1={customData#t1}&t2={customData#t2}&t3={customData#t3}&v1={customData#v1}&p1={customData#p1}&n1={customData#n1}&rssi={rssi}&seqn={seqNumber}&lat={lat}&lng={lng}&snr={snr}&avgsnr={avgSnr}

ukázka GET (ve skuteènosti posílá POST)
GET http://itrubec.cz/monitor/newdata.php?key=xxxxxxx&dev=xxxxx&t1=76&t2=75&t3=75&v1=45&p1=116&n1=50&rssi=-136.00&seqn=23&lat=49.0&lng=17.0&snr=11.84&avgsnr=23.84

Formát zprávy SigFox
t1::int:8 t2::int:8 t3::int:8 v1::int:8 p1::int:8 n1::int:8
*/

$_GET   = filter_input_array(INPUT_GET, FILTER_SANITIZE_STRING);
$_POST  = filter_input_array(INPUT_POST, FILTER_SANITIZE_STRING);

echo("test<br>");

$servername = "xxxx"; //jméno serveru
$username = "xxxxxx"; //DB login
$password = "xxxxxx"; //DB psswd
$dbname = "xxxxxxxx"; //DB name

// Create connection
$mysqli =  new mysqli($servername, $username, $password, $dbname);

/* check connection */
if ($mysqli->connect_errno) {
    printf("Connect failed: %s\n", $mysqli->connect_error);
    exit();
}

/*

STRUKTURA TABULEK

CREATE TABLE IF NOT EXISTS `monitor_devices` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `user` int(11) NOT NULL,
  `device` varchar(255) NOT NULL,
  `devkey` varchar(255) NOT NULL,
  `enabled` tinyint(1) NOT NULL,
  `registered` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `lastactivity` timestamp NULL DEFAULT NULL,
  `note` text,
  PRIMARY KEY (`id`),
  KEY `device` (`device`),
  KEY `key` (`key`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COMMENT='Seznam registrovaných iTRUBEC monitorù pøes SigFox' AUTO_INCREMENT=2 ;


CREATE TABLE IF NOT EXISTS `monitor_data` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `devid` int(11) NOT NULL,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `temp1` int(11) DEFAULT NULL,
  `temp2` int(11) DEFAULT NULL,
  `temp3` int(11) DEFAULT NULL,
  `humidity1` int(11) DEFAULT NULL,
  `pressure1` int(11) DEFAULT NULL,
  `batteryvoltage` float DEFAULT NULL,
  `rssi` float DEFAULT NULL,
  `seqn` bigint(20) DEFAULT NULL,
  `lat` float DEFAULT NULL,
  `lng` float DEFAULT NULL,
  `snr` float DEFAULT NULL,
  `avgsnr` float DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COMMENT='Data iTRUBEC monitorù pøijatá pøes SigFox callBack' AUTO_INCREMENT=1 ;
*/

//http://itrubec.cz/monitor/newdata.php?key=xxxxxxxxx&dev=xxxxxxxxx&t1=76&t2=75&t3=75&v1=45&p1=116&n1=50&rssi=-136.00&seqn=23&lat=49.0&lng=17.0&snr=11.84&avgsnr=23.84
$key = $_POST["key"];
$dev = $_POST["dev"];
$t1 = $_POST["t1"];
$t2 = $_POST["t2"];
$t3 = $_POST["t3"];
$v1 = $_POST["v1"];
$p1 = $_POST["p1"];
$n1 = $_POST["n1"];
$rssi = $_POST["rssi"];
$seqn = $_POST["seqn"];
$lat = $_POST["lat"];
$lng = $_POST["lng"];
$snr = $_POST["snr"];
$avgsnr = $_POST["avgsnr"];

$t1 = $t1 - 50;
$t2 = $t2 - 50;
$t3 = $t3 - 50;
$p1 = $p1 + 850;
$n1 = $n1 / 10;

//SELECT ID FROM `monitor_devices` WHERE devkey like "bflmpsvz" AND device like "38B760"
$sql = "SELECT ID FROM `monitor_devices` WHERE devkey like \"".$key."\" AND device like \"".$dev."\"";
if ($result = $mysqli->query($sql)) {
    printf("Nalezen %d zaznam.\n", $result->num_rows);
    $row=mysqli_fetch_row($result);
    $devid = $row[0];
    $result->close();
    $sql = "UPDATE `monitor_devices` SET `lastactivity`=now() WHERE `id`=".$devid;
    echo($sql);
    echo("<br>");
    if ($mysqli->query($sql) === TRUE) {
      printf("Aktivita zarizeni aktualizovana.<br>\n");
    }
}else{
   die("Bad key or bad device!");
}



$sql = "INSERT INTO `monitor_data`(`devid`, `timestamp`, `temp1`, `temp2`, `temp3`, `humidity1`, `pressure1`, `batteryvoltage`, `rssi`, `seqn`, `lat`, `lng`, `snr`, `avgsnr`) VALUES ($devid, now(),$t1,$t2,$t3,$v1,$p1,$n1,$rssi,$seqn,$lat,$lng,$snr,$avgsnr)";

if ($result = $mysqli->query($sql)) {
    echo "OK";
} else {
    echo "Error" . $sql;
}

$mysqli->close();
?>