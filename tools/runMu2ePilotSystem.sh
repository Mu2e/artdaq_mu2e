#!/bin/bash
export TRACE_FILE=/tmp/trace_buffer_mu2edaq
cd /home/mu2edaq
source /mu2e/ups/setup
setup mu2e_artdaq v1_00_02 -qe7:prof:s15:eth
startMu2ePilotSystem.sh >/dev/null 2>&1 & # The "system" log goes to /daqlogs/pmt
sleep 10
manageMu2ePilotSystem.sh init
runNum=$((1 + `cat runNumbers.log|tail -1|awk '{print $1}'`))
startTime=`date`
echo "$runNum    $startTime" >>runNumbers.log
sleep 10
manageMu2ePilotSystem.sh -N $runNum start
sleep 10800 # Run for 3 hours
manageMu2ePilotSystem.sh stop
sleep 10
manageMu2ePilotSystem.sh shutdown

sleep 5
killall ruby

mkdir -p /home/mu2edaq/daqlogs/traceBuffers/$startTime
cd /home/mu2edaq/daqlogs/traceBuffers/$startTime
pslurp -h /home/mu2edaq/pilotSystem.txt /tmp/trace_buffer_mu2edaq .
