#!/bin/bash
startTime=`date +%Y-%m-%d_%H:%M:%S`

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
   usage: `basename $0` [options]
examples: `basename $0`
          `basename $0` --use-hardware
--use-hardware, -H       Use the DTC hardware (Sets DTCLIB_SIM_ENABLE=N)
--use-rocemulator, -R    Use the ROC Emulator on the DTC hardware (Sets DTCLIB_SIM_ENABLE=R)
--output-path, -o [path] Where to write output data file. If not specified, disk writing will be turned off.
"

# Process script arguments and options
eval env_opts=\${$env_opts_var-} # can be args too
eval "set -- $env_opts \"\$@\""
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`   && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`   && set --  "$rest" "$@"'
reqarg="$op1arg;"'test -z "${1+1}" &&echo opt -$op requires arg. &&echo "$USAGE" &&exit'
args= do_help= opt_h= opt_r= path=
while [ -n "${1-}" ];do
    if expr "x${1-}" : 'x-' >/dev/null;then
        op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
        leq=`expr "x$op" : 'x-[^=]*\(=\)'` lev=`expr "x$op" : 'x-[^=]*=\(.*\)'`
        test -n "$leq"&&eval "set -- \"\$lev\" \"\$@\""&&op=`expr "x$op" : 'x\([^=]*\)'`
        case "$op" in
        \?*|h*)     eval $op1chr; do_help=1;;
        H*|-use-hardware)     eval $op1arg; opt_h=`expr $opt_h + 1`;;
        R*|-use-rocemulator)  eval $op1arg; opt_r=`expr $opt_r + 1`;;
        o*|-output-path)      eval $reqarg; path=$1; shift;;
        *)          echo "Unknown option -$op"; do_help=1;;
        esac
    else
        aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa
set -u   # complain about uninitialed shell variables - helps development

test -n "${do_help-}" -o $# -ge 2 && echo "$USAGE" && exit
test $# -eq 1 && root=$1

if [[ "x$USER" == "xmu2edaq" ]]; then
  mkdir -p /scratch/mu2edaq/daqlogs/cron
  #exec 2>&1
  #exec > >(tee /home/mu2edaq/daqlogs/cron/runMu2ePilotSystem_${startTime}.log)
  exec 2>&1 >/scratch/mu2edaq/daqlogs/cron/runMu2ePilotSystem_${startTime}.log
fi

/home/mu2edaq/cleanupMu2ePilotSystem.sh

hardwareArg=""
if [[ $opt_h -gt 0 ]]; then
  echo "Running with DTC Hardware!!!"
  hardwareArg="-H"
fi
if [[ $opt_r -gt 0 ]]; then
  echo "Running with DTC ROC Emulator!!!"
  hardwareArg="-R"
fi
diskWritingArg="-D"
if [ ! -z "${path:-}" ]; then
  diskWritingArg="-o $path"
fi

export TRACE_FILE=/tmp/trace_buffer_$USER
export RGANG_RSH=/usr/bin/rsh
# mu2edaq04 removed for now
rgang --do-local "mu2edaq0{1,5-8}-data" "/home/mu2edaq/setup_trace.sh"
cd /home/mu2edaq
if [ -z "${MU2E_ARTDAQ_DIR:-}" ]; then
  source /mu2e/ups/setup
  setup mu2e_artdaq -qe10:prof:s41:eth # Use current.chain
fi
if [ ! -z "${MRB_BUILDDIR:-}" ]; then
  export FHICL_FILE_PATH=$FHICL_FILE_PATH:$MRB_BUILDDIR/mu2e_artdaq/fcl
fi

export DTCLIB_SIM_PATH=/home/mu2edaq/data/simData
echo "!!!!STARTING SYSTEM!!!!"
startMu2ePilotSystem.sh & #>/dev/null 2>&1 & # The "system" log goes to /daqlogs/pmt
sleep 30

# No Data File
echo "!!!!!INITIALIZING SYSTEM!!!!!!"
cd /scratch/mu2edaq/daqlogs/cron
#manageMu2ePilotSystem.sh -v -o /mnt/ram/mu2edaq $hardwareArg init
manageMu2ePilotSystem.sh -v $diskWritingArg $hardwareArg init
#manageMu2ePilotSystem.sh -v -D $hardwareArg init
cd /home/mu2edaq
# Data File
#manageMu2ePilotSystem.sh  $hardwareArg init

runNum=$((1 + `cat runNumbers.log|tail -1|awk '{print $1}'`))
echo "$runNum $startTime" >>runNumbers.log
sleep 30
echo "!!!!!!!STARTING RUN!!!!!!!"
manageMu2ePilotSystem.sh -N $runNum start
