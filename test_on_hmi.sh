#!/bin/bash

make clean

make modduo

sshpass -p "mod" scp ./out/mod-controller.bin root@192.168.51.1:/tmp/

sshpass -p 'mod' ssh root@192.168.51.1 hmi-update /tmp/mod-controller.bin

sshpass -p 'mod' ssh root@192.168.51.1 systemctl restart mod-ui
