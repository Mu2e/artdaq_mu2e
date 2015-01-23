#! /bin/env bash

#  This file was created by Ron Rechenmacher <ron@fnal.gov> on Jan 7,
#  2014. "TERMS AND CONDITIONS" governing this file are in the README
#  or COPYING file. If you do not have such a file, one can be
#  obtained by contacting Ron or Fermi Lab in Batavia IL, 60510,
#  phone: 630-840-3000.  $RCSfile: .emacs.gnu,v $
rev='$Revision: 1.20 $$Date: 2010/02/18 13:20:16 $'
#
#  This script is based on the original createArtDaqDemo.sh script created by
#  Kurt and modified by John.

#
# This script will:
#      1.  get (if not already gotten) mu2e_artdaq and all support products
#          Note: this whole demo takes approx. X Megabytes of disk space
#      2a. possibly build the artdaq dependent product
#      2b. build (via cmake),
#  and 3.  start the mu2e_artdaq system
#

# JCF, 1/23/15

# Save all output from this script (stdout + stderr) in a file with a
# name that looks like "quick-start.sh_Fri_Jan_16_13:58:27.script" as
# well as all stderr in a file with a name that looks like
# "quick-start.sh_Fri_Jan_16_13:58:27_stderr.script"

alloutput_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4".script"}' )

stderr_file=$( date | awk -v "SCRIPTNAME=$(basename $0)" '{print SCRIPTNAME"_"$1"_"$2"_"$3"_"$4"_stderr.script"}' )

exec  > >(tee $alloutput_file)
exec 2> >(tee $stderr_file)


# program (default) parameters
root=
tag=

env_opts_var=`basename $0 | sed 's/\.sh$//' | tr 'a-z-' 'A-Z_'`_OPTS
USAGE="\
   usage: `basename $0` [options] [demo_root]
examples: `basename $0` .
          `basename $0` --run-demo
If the \"demo_root\" optional parameter is not supplied, the user will be
prompted for this location.
--run-demo    runs the demo
--debug       perform a debug build
-f            force download
--skip-check  skip the free diskspace check
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
        \?*|h*)     eval $op1chr; do_help=1;;
        v*)         eval $op1chr; opt_v=`expr $opt_v + 1`;;
        x*)         eval $op1chr; set -x;;
        f*)         eval $op1chr; opt_force=1;;
        t*|-tag)    eval $reqarg; tag=$1;    shift;;
        -skip-check)opt_skip_check=1;;
        -run-demo)  opt_run_demo=--run-demo;;
	-debug)     opt_debug=--debug;;
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

#check that $0 is in a git repo
tools_path=`dirname $0`
tools_path=`cd "$tools_path" >/dev/null;pwd`
tools_dir=`basename $tools_path`
git_working_path=`dirname $tools_path`
cd "$git_working_path" >/dev/null
git_working_path=$PWD
git_status=`git status 2>/dev/null`
git_sts=$?
if [ $git_sts -ne 0 -o $tools_dir != tools ];then
    echo problem with git or quick-start.sh script is not from/in a git repository
    exit 1
fi

branch=`git branch | sed -ne '/^\*/{s/^\* *//;p;q}'`
echo the current branch is $branch
if [ "$branch" != '(no branch)' ];then
    test -z "$tag" && tag=`git tag -l 'v[0-9]*' | tail -n1`
    if [ "$tag" != "$branch" ];then
        echo "checking out tag $tag"
        git checkout $tag
    fi
fi

# JCF, 9/19/14

# Now that we've checked out the mu2e_artdaq version we want, make
# sure we know what qualifier is meant to be passed to the
# downloadDeps.sh and installArtDaqDemo.sh scripts below

defaultqual=`grep defaultqual $git_working_path/ups/product_deps | awk '{print $2}' `


vecho() { test $opt_v -gt 0 && echo "$@"; }
starttime=`date`

if [ -z "$root" ];then
    root=`dirname $git_working_path`
    echo the root is $root
fi

test -d "$root" || mkdir -p "$root"
cd $root

free_disk_G=`df -B1G . | awk '/[0-9]%/{print$(NF-2)}'`
if [ -z "${opt_skip_check-}" -a "$free_disk_G" -lt 15 ];then
    echo "Error: insufficient free disk space ($free_disk_G). Min. require = 15"
    echo "Use the --skip-check option to skip free space check."
    exit 1
fi

if [ ! -x $git_working_path/tools/downloadDeps.sh ];then
    echo error: missing tools/downloadDeps.sh
    exit 1
fi
if [ ! -x $git_working_path/tools/installArtDaqDemo.sh ];then
    echo error: missing tools/installArtDaqDemo.sh
    exit 1
fi


if [[ -n "${opt_debug:-}" ]] ; then
    build_type="debug"
else
    build_type="prof"
fi


if [ ! -d products -o ! -d download ];then
    echo "Are you sure you want to download and install the artdaq demo dependent products in `pwd`? [y/n]"
    read response
    if [[ "$response" != "y" ]]; then
        echo "Aborting..."
        exit
    fi
    test -d products || mkdir products
    test -d download || mkdir download
    cd download
    $git_working_path/tools/downloadDeps.sh  ../products $defaultqual $build_type
    cd ..
elif [ -n "${opt_force-}" ];then
    cd download
    $git_working_path/tools/downloadDeps.sh  ../products $defaultqual $build_type
    cd ..
fi


$git_working_path/tools/installArtDaqDemo.sh products $git_working_path ${opt_run_demo-} ${opt_debug-}

endtime=`date`

echo $starttime
echo $endtime



