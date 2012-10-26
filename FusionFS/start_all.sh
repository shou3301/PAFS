#!/bin/sh
for i in 01 02 03 04 
do
	echo
	echo ============ Starting FusionFS on hec-$i
	ssh hec-$i 'cd /home/cshou/FusionFS/fusionFS; ./start.sh; df -lh | grep fusionfs' 
done
