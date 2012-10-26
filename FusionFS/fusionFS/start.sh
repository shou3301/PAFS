#!/bin/sh
LD_LIBRARY_PATH=/mnt/common/cshou/usr/include:/mnt/common/cshou/usr/lib:/home/cshou/FusionFS/fusionFS/src/ffsnet:/home/cshou/FusionFS/fusionFS/src/udt/src:/home/cshou/FusionFS/fusionFS/src/provenance
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH

JAVA_HOME=/mnt/common/cshou/usr/jdk1.7.0
export JAVA_HOME=$JAVA_HOME
PATH=$JAVA_HOME/bin:$PATH
export PATH

/home/cshou/FusionFS/fusionFS/src/fusionfs fusion_root fusion_mount
