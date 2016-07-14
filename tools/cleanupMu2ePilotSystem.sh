#!/bin/bash
startTime=`cat runNumbers.log|tail -1|awk '{print $2}'`
if [[ "x$USER" == "xmu2edaq" ]]; then
  exec 2>&1
  exec > /home/mu2edaq/daqlogs/cron/cleanupMu2ePilotSystem_${startTime}.log
fi
echo "Killing system from $startTime"

cd /home/mu2edaq
if [ -z "$MU2E_ARTDAQ_DIR" ]; then
  source /mu2e/ups/setup
  setup mu2e_artdaq -qe9:prof:s21:eth
fi

manageMu2ePilotSystem.sh stop
sleep 10
manageMu2ePilotSystem.sh shutdown

sleep 5
killall ruby
killall AggregatorMain

export RGANG_RSH=/usr/bin/rsh

rgang "mu2edaq0{4-8}-data" killall BoardReaderMain
rgang "mu2edaq0{4-8}-data" killall EventBuilderMain

mkdir -p /home/mu2edaq/daqlogs/traceBuffers/$startTime

rgang --do-local "mu2edaq0{1,4-8}-data" 'mv /tmp/trace_buffer_$USER /home/mu2edaq/daqlogs/traceBuffers/'"$startTime"'/`hostname`.trace'
rgang --do-local "mu2edaq0{1,4-8}-data" 'cp /proc/trace/buffer /home/mu2edaq/daqlogs/traceBuffers/'"$startTime"'/`hostname`.kernel.trace'
