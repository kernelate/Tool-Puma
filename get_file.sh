#!/bin/bash

(echo "open $1"; echo ; echo "get $2";) | ftp
#`ftp` 192.168.2.190 | echo admin; echo "get diana.jpeg"; 

