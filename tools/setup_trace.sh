#!/bin/bash
#This script sets up TRACE logging for the Mu2e Pilot System.

source /mu2e/ups/setup
setup TRACE v3_05_00a -qe7

export TRACE_FILE=/tmp/trace_buffer_$USER
export TRACE_NAME=MU2EDEV
tonM 1-10
tmode 7

