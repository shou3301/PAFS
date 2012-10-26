#! /bin/sh

for i in 01 02 03 04
do
	echo
	echo "============== copy to hec-$i"
	#ssh hec-$i 'cp /mnt/common/cshou/FusionFS/fusionFS/src/SPADE/bin/FfsConfig.sh /home/cshou/FusionFS/fusionFS/src/SPADE/bin/'
	#ssh hec-$i 'cp /mnt/common/cshou/FusionFS/fusionFS/start.sh /home/cshou/FusionFS/fusionFS'
	ssh hec-$i 'cp /mnt/common/cshou/FusionFS/fusionFS/*.sh /home/cshou/FusionFS/fusionFS'
done
