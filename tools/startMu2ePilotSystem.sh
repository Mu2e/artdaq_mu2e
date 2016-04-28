#!/bin/bash

source `which setupDemoEnvironment.sh`

# create the configuration file for PMT
logroot="/home/mu2edaq/daqlogs"
tempFile="$logroot/pmtConfig.$$"

echo "Currently the Pilot System is a 5x50x2 ARTDAQ configuration"

#echo "BoardReaderMain mu2edaq04-data ${MU2EARTDAQ_BR_PORT[0]}" >> $tempFile
echo "BoardReaderMain mu2edaq05-data ${MU2EARTDAQ_BR_PORT[1]}" >> $tempFile
echo "BoardReaderMain mu2edaq06-data ${MU2EARTDAQ_BR_PORT[2]}" >> $tempFile
echo "BoardReaderMain mu2edaq07-data ${MU2EARTDAQ_BR_PORT[3]}" >> $tempFile
echo "BoardReaderMain mu2edaq08-data ${MU2EARTDAQ_BR_PORT[4]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[0]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[1]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[2]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[3]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[4]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[5]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[6]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[7]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[8]}" >> $tempFile
# echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[9]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[10]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[11]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[12]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[13]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[14]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[15]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[16]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[17]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[18]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[19]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[20]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[21]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[22]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[23]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[24]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[25]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[26]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[27]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[28]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[29]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[30]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[31]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[32]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[33]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[34]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[35]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[36]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[37]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[38]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[39]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[40]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[41]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[42]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[43]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[44]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[45]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[46]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[47]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[48]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[49]}" >> $tempFile
echo "AggregatorMain mu2edaq01-data ${MU2EARTDAQ_AG_PORT[0]}" >> $tempFile
echo "AggregatorMain mu2edaq01-data ${MU2EARTDAQ_AG_PORT[1]}" >> $tempFile

# create the logfile directories, if needed
mkdir -p -m 0777 ${logroot}/pmt
mkdir -p -m 0777 ${logroot}/masterControl
mkdir -p -m 0777 ${logroot}/boardreader
mkdir -p -m 0777 ${logroot}/eventbuilder
mkdir -p -m 0777 ${logroot}/aggregator

# start PMT
pmt.rb -p ${MU2EARTDAQ_PMT_PORT} -d $tempFile --logpath ${logroot} --display ${DISPLAY}
rm $tempFile
