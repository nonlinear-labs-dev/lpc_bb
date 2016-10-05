#!/bin/bash

if [[ -z ${CROSS_COMPILE} ]]; then
	echo You have to set \$CROSS_COMPILE in order to use this build script!
	exit -1
fi

if [[ -z ${KSRC} ]]; then
	echo You have to set \$KSRC to point to your kernel sourcetree, in order to usre this buildscript!
	exit -1
fi


if [[ $1 == "clean" ]]; then
	echo "Cleaning..."
	make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -C ${KSRC} M=$(pwd) clean
elif [[ $1 == "reload" ]]; then
	scp lpc_bb_driver.ko root@192.168.0.234:/lib/modules/3.18.4/extra/lpc_bb_driver.ko
	ssh -l root 192.168.0.234 "killall -9 playground; rmmod lpc_bb_driver; modprobe lpc_bb_driver"
else
	echo "Building..."
	make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -C ${KSRC} M=$(pwd)
fi

