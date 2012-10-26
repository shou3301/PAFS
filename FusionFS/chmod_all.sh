#! /bin/sh

for i in 01 02 03 04
do
	echo
	echo "=================chmod on hec-$i"
	ssh hec-$i 'cd /home/cshou/FusionFS/; chmod 755 *.sh; cd fusionFS; chmod 755 *.sh; chmod 755 src/SPADE/bin/*'
done
