#!/bin/bash
startTime=`cat runNumbers.log|tail -1|cut -f2`
echo "Killing system from $startTime"

cd /home/mu2edaq
if [ -z "$MU2E_ARTDAQ_DIR" ]; then
  source /mu2e/ups/setup
  setup mu2e_artdaq v1_00_02 -qe7:prof:s15:eth
fi

manageMu2ePilotSystem.sh stop
sleep 10
manageMu2ePilotSystem.sh shutdown

sleep 5
killall ruby
killall AggregatorMain

rgang "mu2edaq0{4-8}" killall BoardReaderMain
rgang "mu2edaq0{4-8}" killall EventBuilderMain

mkdir -p /home/mu2edaq/daqlogs/traceBuffers/$startTime

rgang --do-local "mu2edaq0{1,4-8}" 'mv /tmp/trace_buffer_$USER /home/mu2edaq/daqlogs/traceBuffers/$startTime/`hostname`.trace'
