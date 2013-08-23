<html>
<head>
<meta http-equiv="refresh" content="1" />
<title>
Taylor Street and SR87 Intersection
</title>
</head>

<body bgcolor = "white">

<?php
// light images
$ver_top_grn = 'IMAGES/traffic_signal_small_top_green.bmp';
$ver_top_yew = 'IMAGES/traffic_signal_small_top_yellow.bmp';
$ver_top_red = 'IMAGES/traffic_signal_small_top_red.bmp';
$ver_bottom_grn = 'IMAGES/traffic_signal_small_bottom_green.bmp';
$ver_bottom_yew = 'IMAGES/traffic_signal_small_bottom_yellow.bmp';
$ver_bottom_red = 'IMAGES/traffic_signal_small_bottom_red.bmp';
$hl_grn = 'IMAGES/traffic_signal_small_hl_green.bmp';
$hl_yew = 'IMAGES/traffic_signal_small_hl_yellow.bmp';
$hl_red = 'IMAGES/traffic_signal_small_hl_red.bmp';
$hr_grn = 'IMAGES/traffic_signal_small_hr_green.bmp';
$hr_yew = 'IMAGES/traffic_signal_small_hr_yellow.bmp';
$hr_red = 'IMAGES/traffic_signal_small_hr_red.bmp';
//under construction image
$under_construction_sgn = 'IMAGES/under_construction.gif';
?>

<?php
$handle = fopen("/tmp/blah", "r");
$phase1read = fgetc($handle);
$phase2read = fgetc($handle);
$phase3read = fgetc($handle);
$phase4read = fgetc($handle);
$phase5read = fgetc($handle);
$phase6read = fgetc($handle);
$phase7read = fgetc($handle);
$phase8read = fgetc($handle);
//printf("phase1read %c %d", $phase1read, $phase1read);
fclose($handle);
if($phase1read == '2')
	$phase1 = $hr_grn;
if($phase4read == '2')
	$phase4 = $ver_top_grn;
if($phase7read == '2')
	$phase7 = $ver_top_grn;
if($phase6read == '2')
	$phase6 = $hr_grn;
if($phase2read == '2')
	$phase2 = $hl_grn;
if($phase5read == '2')
	$phase5 = $hl_grn;
if($phase8read == '2')
	$phase8 = $ver_bottom_grn;
if($phase3read == '2')
	$phase3 = $ver_bottom_grn;

if($phase1read == '1')
	$phase1 = $hr_yew;
if($phase4read == '1')
	$phase4 = $ver_top_yew;
if($phase7read == '1')
	$phase7 = $ver_top_yew;
if($phase6read == '1')
	$phase6 = $hr_yew;
if($phase2read == '1')
	$phase2 = $hl_yew;
if($phase5read == '1')
	$phase5 = $hl_yew;
if($phase8read == '1')
	$phase8 = $ver_bottom_yew;
if($phase3read == '1')
	$phase3 = $ver_bottom_yew;

if($phase1read == '0')
	$phase1 = $hr_red;
if($phase4read == '0')
	$phase4 = $ver_top_red;
if($phase7read == '0')
	$phase7 = $ver_top_red;
if($phase6read == '0')
	$phase6 = $hr_red;
if($phase2read == '0')
	$phase2 = $hl_red;
if($phase5read == '0')
	$phase5 = $hl_red;
if($phase8read == '0')
	$phase8 = $ver_bottom_red;
if($phase3read == '0')
	$phase3 = $ver_bottom_red;

 	echo '<td>&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp<img src= '.$phase4.' /><img src= '.$phase7.' /></br></br></td >';
	echo '<td>&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp<img src= '.$phase6.' /></br></br></td >';
 	echo '<td><img src= '.$phase5.' />&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp<img src= '.$phase1.' /></br></br></td >';
 	echo '<td><img src= '.$phase2.' /></br></br></td>';
 	echo '<td>&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp<img src= '.$phase8.' /><img src= '.$phase3.' /></td >';
?>
</body>
</html>
