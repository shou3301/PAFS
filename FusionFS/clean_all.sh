#!/bin/sh
for i in 01 02 03 04 
do
	echo
	echo ============ Cleaning SPADE records on hec-$i
	ssh hec-$i 'rm -r /home/cshou/database; cd /home/cshou/FusionFS/fusionFS/src/SPADE/cfg; rm spade.config; touch spade.config' 
done