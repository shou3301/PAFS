#!/bin/sh
for i in 01 02 03 04 
do
	echo
	echo ============ Stoping FusionFS on hec-$i
	ssh hec-$i 'cd /home/cshou/FusionFS/fusionFS; ./stop.sh' 
done