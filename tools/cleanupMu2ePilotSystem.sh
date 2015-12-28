#!/bin/bash
startTime=`cat runNumbers.log|tail -1|cut -f2`
echo "Killing system from $startTime"

cd /home/mu2edaq
source /mu2e/ups/setup
setup mu2e_artdaq v1_00_02 -qe7:prof:s15:eth

manageMu2ePilotSystem.sh stop
sleep 10
manageMu2ePilotSystem.sh shutdown

sleep 5
killall ruby

unsetup_all
/home/mu2edaq/cleanup_trace.sh $startTime
