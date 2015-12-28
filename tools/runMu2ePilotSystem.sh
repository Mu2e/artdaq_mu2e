#!/bin/bash
export TRACE_FILE=/tmp/trace_buffer_$USER
pssh -i -h /home/mu2edaq/pilotSystem.txt "/home/mu2edaq/setup_trace.sh"
cd /home/mu2edaq
if [ -z "$MU2E_ARTDAQ_DIR" ]; then
  source /mu2e/ups/setup
  setup mu2e_artdaq v1_00_02 -qe7:prof:s15:eth
fi
startMu2ePilotSystem.sh >/dev/null 2>&1 & # The "system" log goes to /daqlogs/pmt
sleep 10
manageMu2ePilotSystem.sh init
runNum=$((1 + `cat runNumbers.log|tail -1|awk '{print $1}'`))
startTime=`date +%Y-%m-%d_%H:%M:%S`
echo "$runNum	$startTime" >>runNumbers.log
sleep 10
manageMu2ePilotSystem.sh -N $runNum start
