#!/bin/bash
pkill ffsnetd
pkill server_zht

#stop service for SPADE server
./src/SPADE/bin/server stop
