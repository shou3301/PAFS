LD_LIBRARY_PATH=/mnt/common/cshou/usr/include:/mnt/common/cshou/usr/lib:/home/cshou/FusionFS/fusionFS/src/ffsnet:/home/cshou/FusionFS/fusionFS/src/udt/src:/home/cshou/FusionFS/fusionFS/src/provenance
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH

JAVA_HOME=/mnt/common/cshou/usr/jdk1.7.0
export JAVA_HOME=$JAVA_HOME
PATH=$JAVA_HOME/bin:$PATH
export PATH

echo $LD_LIBRARY_PATH

#start the service for file transfer
/home/cshou/FusionFS/fusionFS/src/ffsnet/ffsnetd 2>&1 1>/dev/null &  

#start the service for ZHT server
/home/cshou/FusionFS/fusionFS/src/zht/bin/server_zht 50000 /home/cshou/FusionFS/fusionFS/src/zht/neighbor /home/cshou/FusionFS/fusionFS/src/zht/zht.cfg TCP 2>&1 1>/dev/null &

#start the service for SPADE server
/home/cshou/FusionFS/fusionFS/src/SPADE/bin/server start
