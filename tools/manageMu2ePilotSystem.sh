#!/bin/bash

source `which setupDemoEnvironment.sh` ""

AGGREGATOR_NODE=mu2edaq01.fnal.gov
THIS_NODE=`hostname -s`

# this function expects a number of arguments:
#  1) the DAQ command to be sent
#  2) the run number (dummy for non-start commands)
#  3) whether to run online monitoring
#  4) the data directory
#  5) the logfile name
#  6) whether to write data to disk [0,1]
#  7) the desired number of events in the run (stop command)
#  8) the desired duration of the run (minutes, stop command)
#  9) the desired size of each data file
# 10) the desired number of events in each file
# 11) the desired time duration of each file (minutes)
# 12) whether to print out CFG information (verbose)
# 13) whether we're in hardware or software mode
function launch() {

  enableSerial=""
  if [[ "${12}" == "1" ]]; then
      enableSerial="-e"
  fi

  CAL=2
  TRK=1
  USEFILE=1
  if [[ "${13}" == "1" ]]; then
    CAL=4
    TRK=4
  fi
  if [[ "${13}" == "2" ]]; then
    CAL=5
    TRK=5
    USEFILE=0
  fi

# mu2edaq04 removed for now
    # --mu2e mu2edaq04-data,${MU2EARTDAQ_BR_PORT[0]},0,${TRK},1,$DTCLIB_SIM_PATH/mediumPackets.bin \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[0]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[5]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[10]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[15]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[20]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[25]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[30]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[35]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[40]} \
    # --eb mu2edaq04-data,${MU2EARTDAQ_EB_PORT[45]} \


  DemoControl.rb ${enableSerial} -s -c $1 \
    --mu2e mu2edaq05-data,${MU2EARTDAQ_BR_PORT[1]},1,${TRK},${USEFILE},$DTCLIB_SIM_PATH/mediumPackets.bin \
    --mu2e mu2edaq06-data,${MU2EARTDAQ_BR_PORT[2]},2,${TRK},${USEFILE},$DTCLIB_SIM_PATH/mediumPackets.bin \
    --mu2e mu2edaq07-data,${MU2EARTDAQ_BR_PORT[3]},3,${TRK},${USEFILE},$DTCLIB_SIM_PATH/mediumPackets.bin \
    --mu2e mu2edaq08-data,${MU2EARTDAQ_BR_PORT[4]},4,${CAL},${USEFILE},$DTCLIB_SIM_PATH/mediumPackets.bin \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[1]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[2]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[3]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[4]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[6]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[7]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[8]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[9]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[11]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[12]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[13]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[14]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[16]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[17]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[18]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[19]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[21]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[22]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[23]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[24]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[26]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[27]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[28]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[29]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[31]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[32]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[33]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[34]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[36]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[37]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[38]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[39]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[41]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[42]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[43]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[44]} \
    --eb mu2edaq05-data,${MU2EARTDAQ_EB_PORT[46]} \
    --eb mu2edaq06-data,${MU2EARTDAQ_EB_PORT[47]} \
    --eb mu2edaq07-data,${MU2EARTDAQ_EB_PORT[48]} \
    --eb mu2edaq08-data,${MU2EARTDAQ_EB_PORT[49]} \
    --ag mu2edaq01-data,${MU2EARTDAQ_AG_PORT[0]},1 \
    --ag mu2edaq01-data,${MU2EARTDAQ_AG_PORT[1]},1 \
    --data-dir ${4} --online-monitoring $3 \
    --write-data ${6} --run-event-count ${7} \
    --run-duration ${8} --file-size ${9} \
    --file-event-count ${10} --file-duration ${11} \
    --run-number $2  2>&1 | tee -a ${5}
}

scriptName=`basename $0`
usage () {
    echo "
Usage: ${scriptName} [options] <command>
Where command is one of:
  init, start, pause, resume, stop, status, get-legal-commands,
  shutdown, start-system, restart, reinit, exit,
  fast-shutdown, fast-restart, fast-reinit, or fast-exit
General options:
  -h, --help: prints this usage message
Configuration options (init commands):
  -m <on|off>: specifies whether to run online monitoring [default=off]
  -D : disables the writing of data to disk
  -s <file size>: specifies the size threshold for closing data files (in MB)
      [default is 8000 MB (~7.8 GB); zero means that there is no file size limit]
  --file-events <count>: specifies the desired number of events in each file
      [default=0, which means no event count limit for files]
  --file-duration <duration>: specifies the desired duration of each file (minutes)
      [default=0, which means no duration limit for files]
  -o <data dir>: specifies the directory for data files [default=/tmp]
  -H: Hardware mode (Currently using Detector Emulation mode of the DTC)
Begin-run options (start command):
  -N <run number>: specifies the run number
End-run options (stop command):
  -n, --run-events <count>: specifies the desired number of events in the run
      [default=0, which ends the run immediately, independent of the number of events]
  -d, --run-duration <duration>: specifies the desired length of the run (minutes)
      [default=0, which ends the run immediately, independent of the duration of the run]
Notes:
  The start command expects a run number to be specified (-N).
  The primary commands are the following:
   * init - initializes (configures) the DAQ processes
   * start - starts a run
   * pause - pauses the run
   * resume - resumes the run
   * stop - stops the run
   * status - checks the status of each DAQ process
   * get-legal-commands - fetches the legal commands from each DAQ process
  Additional commands include:
   * shutdown - stops the run (if one is going), resets the DAQ processes
       to their ground state (if needed), and stops the MPI program (DAQ processes)
   * start-system - starts the MPI program (the DAQ processes)
   * restart - this is the same as a shutdown followed by a start-system
   * reinit - this is the same as a shutdown followed by a start-system and an init
   * exit - this resets the DAQ processes to their ground state, stops the MPI
       program, and exits PMT.
  Expert-level commands:
   * fast-shutdown - stops the MPI program (all DAQ processes) no matter what
       state they are in. This could have bad consequences if a run is going!
   * fast-restart - this is the same as a fast-shutdown followed by a start-system
   * fast-reinit - this is the same as a fast-shutdown followed by a start-system
       and an init
   * fast-exit - this stops the MPI program, and exits PMT.
Examples: ${scriptName} -p 32768 init
          ${scriptName} -N 101 start
" >&2
}

# parse the command-line options
originalCommand="$0 $*"
onmonEnable=off
diskWriting=1
dataDir="/home/mu2edaq/data"
runNumber=""
runEventCount=0
runDuration=0
fileSize=8000
fsChoiceSpecified=0
fileEventCount=0
fileDuration=0
hardwareMode=0
verbose=0
OPTIND=1
while getopts "hHRc:N:o:t:m:Dn:d:s:w:v-:" opt; do
    if [ "$opt" = "-" ]; then
        opt=$OPTARG
    fi
    case $opt in
        h | help)
            usage
            exit 1
            ;;
        N)
            runNumber=${OPTARG}
            ;;
        m)
            onmonEnable=${OPTARG}
            ;;
        o)
            dataDir=${OPTARG}
            ;;
        D)
            diskWriting=0
            ;;
	H)
	    hardwareMode=1
	    ;;
	R)
	    hardwareMode=2
	    ;;
        n)
            runEventCount=${OPTARG}
            ;;
        d)
            runDuration=${OPTARG}
            ;;
        run-events)
            runEventCount=${!OPTIND}
            let OPTIND=$OPTIND+1
            ;;
        run-duration)
            runDuration=${!OPTIND}
            let OPTIND=$OPTIND+1
            ;;
        file-events)
            fileEventCount=${!OPTIND}
            let OPTIND=$OPTIND+1
            ;;
        file-duration)
            fileDuration=${!OPTIND}
            let OPTIND=$OPTIND+1
            ;;
        s)
            fileSize=${OPTARG}
            fsChoiceSpecified=1
            ;;
        v)
            verbose=1
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done
shift $(($OPTIND - 1))

# fetch the command to run
if [ $# -lt 1 ]; then
    usage
    exit
fi
command=$1
shift

# verify that the command is one that we expect
if [[ "$command" != "start-system" ]] && \
   [[ "$command" != "init" ]] && \
   [[ "$command" != "start" ]] && \
   [[ "$command" != "pause" ]] && \
   [[ "$command" != "resume" ]] && \
   [[ "$command" != "stop" ]] && \
   [[ "$command" != "status" ]] && \
   [[ "$command" != "get-legal-commands" ]] && \
   [[ "$command" != "shutdown" ]] && \
   [[ "$command" != "fast-shutdown" ]] && \
   [[ "$command" != "restart" ]] && \
   [[ "$command" != "fast-restart" ]] && \
   [[ "$command" != "reinit" ]] && \
   [[ "$command" != "fast-reinit" ]] && \
   [[ "$command" != "exit" ]] && \
   [[ "$command" != "fast-exit" ]]; then
    echo "Invalid command."
    usage
    exit
fi

# verify that the expected arguments were provided
if [[ "$command" == "start" ]] && [[ "$runNumber" == "" ]]; then
    echo ""
    echo "*** A run number needs to be specified."
    usage
    exit
fi

# fill in values for options that weren't specified
if [[ "$runNumber" == "" ]]; then
    runNumber=101
fi

# translate the onmon enable flag
if [[ "$onmonEnable" == "on" ]]; then
    onmonEnable=1
else
    onmonEnable=0
fi

# verify that we don't have both a number of events and a time limit
# for stopping a run
if [[ "$command" == "stop" ]]; then
    if [[ "$runEventCount" != "0" ]] && [[ "$runDuration" != "0" ]]; then
        echo ""
        echo "*** Only one of event count or run duration may be specified."
        usage
        exit
    fi
fi

# build the logfile name
TIMESTAMP=`date '+%Y%m%d%H%M%S'`
logFile="/home/mu2edaq/daqlogs/masterControl/dsMC-${TIMESTAMP}-${command}.log"
echo "${originalCommand}" > $logFile
echo ">>> ${originalCommand} (Disk writing is ${diskWriting})"

# calculate the shmkey that should be checked
let shmKey=1078394880+${MU2EARTDAQ_PMT_PORT}
shmKeyString=`printf "0x%x" ${shmKey}`

# invoke the requested command
if [[ "$command" == "shutdown" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
    # next send a shutdown command to move the processes to their ground state
    launch "shutdown" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.stopSystem
    # clean up any stale shared memory segment
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
elif [[ "$command" == "start-system" ]]; then
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.startSystem
elif [[ "$command" == "restart" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
    # next send a shutdown command to move the processes to their ground state
    launch "shutdown" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.stopSystem
    # clean up any stale shared memory segment
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    # start the MPI program
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.startSystem
elif [[ "$command" == "reinit" ]]; then
    # first send a stop command to end the run (in case it is needed)
    launch "stop" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
    # next send a shutdown command to move the processes to their ground state
    launch "shutdown" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
    # stop the MPI program
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.stopSystem
    # clean up any stale shared memory segment
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    # start the MPI program
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.startSystem
    # send the init command to re-initialize the system
    sleep 5
    launch "init" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
elif [[ "$command" == "exit" ]]; then
    launch "shutdown" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.stopSystem
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.exit
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"

elif [[ "$command" == "fast-shutdown" ]]; then
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.stopSystem
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
elif [[ "$command" == "fast-restart" ]]; then
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.stopSystem
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.startSystem
elif [[ "$command" == "fast-reinit" ]]; then
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.stopSystem
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.startSystem
    sleep 5
    launch "init" $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
elif [[ "$command" == "fast-exit" ]]; then
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.stopSystem
    xmlrpc ${THIS_NODE}:${MU2EARTDAQ_PMT_PORT}/RPC2 pmt.exit
    ssh ${AGGREGATOR_NODE} "ipcs | grep ${shmKeyString} | awk '{print \$2}' | xargs ipcrm -m 2>/dev/null"

else
    launch $command $runNumber $onmonEnable $dataDir \
        $logFile $diskWriting $runEventCount $runDuration $fileSize \
        $fileEventCount $fileDuration $verbose $hardwareMode
fi
