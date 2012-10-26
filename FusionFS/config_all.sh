#!/bin/sh
for i in 01 02 03 04 
do
	echo
	echo ============ Configing SPADE on hec-$i
	ssh hec-$i 'cd /home/cshou/FusionFS/fusionFS/src/SPADE/bin; ./FfsConfig.sh' 
done