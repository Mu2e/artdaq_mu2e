#!/bin/bash
startTime=`date +%Y-%m-%d_%H:%M:%S`
if [[ "x$USER" == "xmu2edaq" ]]; then
  mkdir -p /home/mu2edaq/daqlogs/cron
  exec 2>&1
  exec > /home/mu2edaq/daqlogs/cron/runMu2ePilotSystem_${startTime}.log
fi

/home/mu2edaq/cleanupMu2ePilotSystem.sh

hardwareArg=""
if [[ "x$1" == "xhardware" ]]; then
  echo "Running with DTC Hardware!!!"
  hardwareArg="-H"
fi

export TRACE_FILE=/tmp/trace_buffer_$USER
export RGANG_RSH=/usr/bin/rsh
# mu2edaq04 removed for now
rgang --do-local "mu2edaq0{1,5-8}-data" "/home/mu2edaq/setup_trace.sh"
cd /home/mu2edaq
if [ -z "$MU2E_ARTDAQ_DIR" ]; then
  source /mu2e/ups/setup
  setup mu2e_artdaq -qe9:prof:s21:eth # Use current.chain
fi
if [ ! -z "$MRB_BUILDDIR" ]; then
  export FHICL_FILE_PATH=$FHICL_FILE_PATH:$MRB_BUILDDIR/mu2e_artdaq/fcl
fi

export DTCLIB_SIM_PATH=/home/mu2edaq/data/simData
startMu2ePilotSystem.sh >/dev/null 2>&1 & # The "system" log goes to /daqlogs/pmt
sleep 10

# No Data File
manageMu2ePilotSystem.sh -D $hardwareArg init
# Data File
#manageMu2ePilotSystem.sh  $hardwareArg init

runNum=$((1 + `cat runNumbers.log|tail -1|awk '{print $1}'`))
echo "$runNum $startTime" >>runNumbers.log
sleep 30
manageMu2ePilotSystem.sh -N $runNum start
