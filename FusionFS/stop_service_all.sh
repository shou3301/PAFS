#!/bin/sh
for i in 01 02 03 04
do
	echo
	echo ======================== Stoping service on hec-$i
	ssh hec-$i 'cd /home/cshou/FusionFS/fusionFS; ./stop_service.sh' 
done