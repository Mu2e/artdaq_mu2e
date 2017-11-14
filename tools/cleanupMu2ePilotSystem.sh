#!/bin/bash

logdir=/scratch-mu2edaq01/mu2edaq/daqlogs

startTime=`cat runNumbers.log|tail -1|awk '{print $2}'`
if [[ "x$USER" == "xmu2edaq" ]]; then
  exec 2>&1
  exec > ${logdir}/cron/cleanupMu2ePilotSystem_${startTime}.log
fi
echo "Killing system from $startTime"

cd /home/mu2edaq
if [ -z "$MU2E_ARTDAQ_DIR" ]; then
  source /mu2e/ups/setup
  setup mu2e_artdaq -qe14:prof:s50:eth
fi

manageMu2ePilotSystem.sh stop
sleep 10
manageMu2ePilotSystem.sh shutdown

sleep 20
killall ruby
killall AggregatorMain

export RGANG_RSH=/usr/bin/rsh

rgang "mu2edaq0{4-8}" killall ruby >/dev/null 2>&1
rgang "mu2edaq0{4-8}" killall BoardReaderMain >/dev/null 2>&1
rgang "mu2edaq0{4-8}" killall EventBuilderMain >/dev/null 2>&1

rgang "mu2edaq{10-11}" killall ruby >/dev/null 2>&1
rgang "mu2edaq{10-11}" killall BoardReaderMain >/dev/null 2>&1
rgang "mu2edaq{10-11}" killall EventBuilderMain >/dev/null 2>&1
mkdir -p ${logdir}/traceBuffers/$startTime

rgang --do-local "mu2edaq0{1,4-8}" 'if [ -e /tmp/trace_buffer_$USER ];then mv /tmp/trace_buffer_$USER '"${logdir}"'/traceBuffers/'"$startTime"'/`hostname`.trace; fi'
rgang --do-local "mu2edaq0{1,4-8}" 'if [ -e /proc/trace/buffer ];then cp /proc/trace/buffer '"${logdir}"'/traceBuffers/'"$startTime"'/`hostname`.kernel.trace; fi'
rgang --do-local "mu2edaq{10-11}" 'if [ -e /tmp/trace_buffer_$USER ];then mv /tmp/trace_buffer_$USER '"${logdir}"'/traceBuffers/'"$startTime"'/`hostname`.trace; fi'
rgang --do-local "mu2edaq{10-11}" 'if [ -e /proc/trace/buffer ];then cp /proc/trace/buffer '"${logdir}"'/traceBuffers/'"$startTime"'/`hostname`.kernel.trace; fi'
