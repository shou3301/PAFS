#!/bin/sh
for i in 01
do
	echo
	echo ======================== Starting service on hec-$i
	ssh hec-$i '/home/cshou/FusionFS/fusionFS/start_service.sh </dev/null >/dev/null; ps | grep -e ffsnetd -e server_zht; jps;' 
done
