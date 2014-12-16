#!/bin/bash

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
  usage: `basename $0` [options] <demo_products_dir/> <mu2e_artdaq/>
example: `basename $0` products mu2e_artdaq --run-demo
<demo_products>    where products were installed (products/)
<mu2e_artdaq_root> directory where mu2e_artdaq was cloned into.
--run-demo   runs the demo
Currently this script will clone (if not already cloned) artdaq
along side of the mu2e_artdaq dir.
Also it will create, if not already created, build directories
for artdaq and mu2e_artdaq.
"
# Process script arguments and options
eval env_opts=\${$env_opts_var-} # can be args too
eval "set -- $env_opts \"\$@\""
op1chr='rest=`expr "$op" : "[^-]\(.*\)"`   && set -- "-$rest" "$@"'
op1arg='rest=`expr "$op" : "[^-]\(.*\)"`   && set --  "$rest" "$@"'
reqarg="$op1arg;"'test -z "${1+1}" &&echo opt -$op requires arg. &&echo "$USAGE" &&exit'
args= do_help= opt_v=0
while [ -n "${1-}" ];do
    if expr "x${1-}" : 'x-' >/dev/null;then
        op=`expr "x$1" : 'x-\(.*\)'`; shift   # done with $1
        leq=`expr "x$op" : 'x-[^=]*\(=\)'` lev=`expr "x$op" : 'x-[^=]*=\(.*\)'`
        test -n "$leq"&&eval "set -- \"\$lev\" \"\$@\""&&op=`expr "x$op" : 'x\([^=]*\)'`
        case "$op" in
        \?*|h*)    eval $op1chr; do_help=1;;
        v*)        eval $op1chr; opt_v=`expr $opt_v + 1`;;
        x*)        eval $op1chr; set -x;;
        -run-demo) opt_run_demo=--run-demo;;
        -debug)    opt_debug=--debug;;
        *)         echo "Unknown option -$op"; do_help=1;;
        esac
    else
        aa=`echo "$1" | sed -e"s/'/'\"'\"'/g"` args="$args '$aa'"; shift
    fi
done
eval "set -- $args \"\$@\""; unset args aa

test -n "${do_help-}" -o $# -ne 2 && echo "$USAGE" && exit

products_dir=`cd "$1" >/dev/null;pwd`
mu2e_artdaq_dir=`cd "$2" >/dev/null;pwd`
demo_dir=`dirname "$mu2e_artdaq_dir"`

export CETPKG_INSTALL=$products_dir
export CETPKG_J=16

test -d "$demo_dir/build_artdaq-core" || mkdir "$demo_dir/build_artdaq-core" 
test -d "$demo_dir/build_artdaq"      || mkdir "$demo_dir/build_artdaq"  # This is where we will build artdaq
test -d "$demo_dir/build_mu2e_artdaq" || mkdir "$demo_dir/build_mu2e_artdaq"  # This is where we will build mu2e_artdaq

if [[ -n "${opt_debug:-}" ]];then
    build_arg="d"
else
    build_arg="p"
fi

test -d artdaq-core || git clone http://cdcvs.fnal.gov/projects/artdaq-core
cd artdaq-core
git fetch origin
git checkout v1_04_05
cd ../build_artdaq-core
echo IN $PWD: about to . ../artdaq-core/ups/setup_for_development
. $products_dir/setup
. ../artdaq-core/ups/setup_for_development -${build_arg} e6 s5
echo FINISHED ../artdaq-core/ups/setup_for_development
buildtool -i
cd ..


test -d artdaq || git clone http://cdcvs.fnal.gov/projects/artdaq
cd artdaq
git fetch origin
git checkout v1_12_03
cd ../build_artdaq
echo IN $PWD: about to . ../artdaq/ups/setup_for_development
. $products_dir/setup
. ../artdaq/ups/setup_for_development -${build_arg} e6 s5 eth
echo FINISHED ../artdaq/ups/setup_for_development
buildtool -i

cd $demo_dir >/dev/null
if [[ ! -e ./setupMU2EARTDAQ ]]; then
    cat >setupMU2EARTDAQ <<-EOF
	echo # This script is intended to be sourced.

	sh -c "[ \`ps \$\$ | grep bash | wc -l\` -gt 0 ] || { echo 'Please switch to the bash shell before running the mu2e_artdaq.'; exit; }" || exit

	source $products_dir/setup

	export CETPKG_INSTALL=$products_dir
	export CETPKG_J=16
	#export MU2EARTDAQ_BASE_PORT=52200
	export DAQ_INDATA_PATH=$mu2e_artdaq_dir/test/Generators:$mu2e_artdaq_dir/inputData

	export MU2EARTDAQ_BUILD="$demo_dir/build_mu2e_artdaq"
	export MU2EARTDAQ_REPO="$mu2e_artdaq_dir"
	export FHICL_FILE_PATH=.:\$MU2EARTDAQ_REPO/tools/fcl:\$FHICL_FILE_PATH

	echo changing directory to \$MU2EARTDAQ_BUILD
	cd \$MU2EARTDAQ_BUILD  # note: next line adjusts PATH based one cwd
#	. \$MU2EARTDAQ_REPO/ups/setup_for_development -p e5 eth
	. \$MU2EARTDAQ_REPO/ups/setup_for_development -p

	alias rawEventDump="art -c $mu2e_artdaq_dir/mu2e_artdaq/ArtModules/fcl/rawEventDump.fcl"
	alias compressedEventDump="art -c $mu2e_artdaq_dir/mu2e_artdaq/ArtModules/fcl/compressedEventDump.fcl"
	alias compressedEventComparison="art -c $mu2e_artdaq_dir/mu2e_artdaq/ArtModules/fcl/compressedEventComparison.fcl"
	EOF
    #
fi


echo "Building mu2e_artdaq..."
cd $MU2EARTDAQ_BUILD
. $demo_dir/setupMU2EARTDAQ
buildtool

echo "Installation and build complete"

if [ -n "${opt_run_demo-}" ];then
    echo doing the demo

    $mu2e_artdaq_dir/tools/xt_cmd.sh $demo_dir --geom 132x33 \
        -c '. ./setupMU2EARTDAQ' \
        -c start2x2x2System.sh
    sleep 2

    $mu2e_artdaq_dir/tools/xt_cmd.sh $demo_dir --geom 132 \
        -c '. ./setupMU2EARTDAQ' \
        -c ':,sleep 10' \
        -c 'manage2x2x2System.sh -m on init' \
        -c ':,sleep 5' \
        -c 'manage2x2x2System.sh -N 101 start' \
        -c ':,sleep 10' \
        -c 'manage2x2x2System.sh stop' \
        -c ':,sleep 5' \
        -c 'manage2x2x2System.sh shutdown' \
        -c ': For additional commands, see output from: manage2x2x2System.sh --help' \
        -c ':: manage2x2x2System.sh --help' \
        -c ':: manage2x2x2System.sh exit'
fi
