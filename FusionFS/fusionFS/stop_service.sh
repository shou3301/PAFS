#!/bin/bash
pkill ffsnetd
pkill server_zht

#stop service for SPADE server
/home/cshou/FusionFS/fusionFS/src/SPADE/bin/server stop
