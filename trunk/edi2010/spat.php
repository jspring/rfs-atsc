<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="refresh" content="1" />  
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<META HTTP-EQUIV="CACHE-CONTROL" CONTENT="NO-CACHE" />
<title>SPAT</title>
</head>
<body>

<h1 align="center">Signal Phase</h1>

<?php
$p2fd=fopen("/opt/lampp/htdocs/p2_color.txt","r");
$p2ret=fscanf($p2fd,"%d");
$left_light = $p2ret[0];

switch($left_light)
{
case "2":
  $light=Green;
	break;
case "1":
  $light=Yellow;
	break;
case "0":
  $light=Red;
	break;
}
$leftColor = $light;
fclose($p2fd);

$p4fd=fopen("/opt/lampp/htdocs/p4_color.txt","r");
$p4ret = fscanf($p4fd,"%d");
$right_light = $p4ret[0];
switch($right_light)
{
case "2":
  $light=Green;
	break;
case "1":
  $light=Yellow;
	break;
case "0":
  $light=Red;
	break;
}
$rightColor = $light;
fclose($p4fd);
?>

<table align="center" border="2">
	<tr>
		<td align="center"></td>
		<td width="150" align="center">
		<?php 
			if ($leftColor == Green)
//				echo "<img src='/spat_files/traffic_signal_small_top_green.bmp' />";
				echo "<img src='/spat_files/traffic_signal_large_green.bmp' />";
			else if ($leftColor == Yellow)
				echo "<img src='/spat_files/traffic_signal_large_yellow.bmp' />";
			else if ($leftColor == Red)
				echo "<img src='/spat_files/traffic_signal_large_red.bmp' />";					
		?>
		</td>
		<td align="center"></td>
	</tr>
	<tr>
		<td width="150" align="center">
		<?php 		
			if ($rightColor == Green)
				echo "<img src='/spat_files/traffic_signal_large_green.bmp' /><br />";
			else if ($rightColor == Yellow)
				echo "<img src='/spat_files/traffic_signal_large_yellow.bmp' /><br />";
			else if ($rightColor == Red)
				echo "<img src='/spat_files/traffic_signal_large_red.bmp' /><br />";
		?>
		</td>		
		<td width="150" align="center">
		<?php
			echo '<h3 align="center">Scan Mode' .$newline .$newline .$right_control_mode;"</h3>"

		?>
		</td>
		<td width="150" align="center">
		<?php 
			if ($rightColor == Green)
				echo "<img src='/spat_files/traffic_signal_large_green.bmp' /><br />";
			else if ($rightColor == Yellow)
				echo "<img src='/spat_files/traffic_signal_large_yellow.bmp' /><br />";
			else if ($rightColor == Red)
				echo "<img src='/spat_files/traffic_signal_large_red.bmp' /><br />";		
		?>
		</td>		
	</tr>
	<tr>
		<td align="center"></td>
		<td width="150" align="center">
		<?php 
			if ($leftColor == Green)
				echo "<img src='/spat_files/traffic_signal_large_green.bmp' /><br />";
			else if ($leftColor == Yellow)
				echo "<img src='/spat_files/traffic_signal_large_yellow.bmp' /><br />";
			else if ($leftColor == Red)
				echo "<img src='/spat_files/traffic_signal_large_red.bmp' /><br />";
		?>
		</td>
		<td align="center"></td>		
	</tr>
</table>

<h2 align="center">Signal Phase</h2>

</body>
</html>
