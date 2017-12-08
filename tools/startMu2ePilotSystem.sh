#!/bin/bash

source `which setupDemoEnvironment.sh`

# create the configuration file for PMT
logroot="/scratch-mu2edaq01/mu2edaq/daqlogs"
tempFile="$logroot/pmtConfig.$$"

echo "Currently the Pilot System is a 8x80x2 ARTDAQ configuration"

echo "BoardReaderMain mu2edaq03-data ${MU2EARTDAQ_BR_PORT[0]}" >> $tempFile
echo "BoardReaderMain mu2edaq04-data ${MU2EARTDAQ_BR_PORT[1]}" >> $tempFile
echo "BoardReaderMain mu2edaq05-data ${MU2EARTDAQ_BR_PORT[2]}" >> $tempFile
echo "BoardReaderMain mu2edaq06-data ${MU2EARTDAQ_BR_PORT[3]}" >> $tempFile
echo "BoardReaderMain mu2edaq07-data ${MU2EARTDAQ_BR_PORT[4]}" >> $tempFile
echo "BoardReaderMain mu2edaq08-data ${MU2EARTDAQ_BR_PORT[5]}" >> $tempFile
echo "BoardReaderMain mu2edaq10-data ${MU2EARTDAQ_BR_PORT[6]}" >> $tempFile
echo "BoardReaderMain mu2edaq11-data ${MU2EARTDAQ_BR_PORT[7]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[0]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[1]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[2]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[3]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[4]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[5]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[6]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[7]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[10]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[11]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[12]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[13]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[14]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[15]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[16]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[17]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[20]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[21]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[22]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[23]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[24]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[25]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[26]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[27]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[30]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[31]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[32]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[33]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[34]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[35]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[36]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[37]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[40]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[41]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[42]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[43]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[44]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[45]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[46]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[47]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[50]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[51]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[52]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[53]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[54]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[55]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[56]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[57]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[60]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[61]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[62]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[63]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[64]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[65]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[66]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[67]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[70]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[71]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[72]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[73]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[74]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[75]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[76]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[77]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[80]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[81]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[82]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[83]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[84]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[85]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[86]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[87]}" >> $tempFile

echo "EventBuilderMain mu2edaq03-data ${MU2EARTDAQ_EB_PORT[90]}" >> $tempFile
echo "EventBuilderMain mu2edaq04-data ${MU2EARTDAQ_EB_PORT[91]}" >> $tempFile
echo "EventBuilderMain mu2edaq05-data ${MU2EARTDAQ_EB_PORT[92]}" >> $tempFile
echo "EventBuilderMain mu2edaq06-data ${MU2EARTDAQ_EB_PORT[93]}" >> $tempFile
echo "EventBuilderMain mu2edaq07-data ${MU2EARTDAQ_EB_PORT[94]}" >> $tempFile
echo "EventBuilderMain mu2edaq08-data ${MU2EARTDAQ_EB_PORT[95]}" >> $tempFile
echo "EventBuilderMain mu2edaq10-data ${MU2EARTDAQ_EB_PORT[96]}" >> $tempFile
echo "EventBuilderMain mu2edaq11-data ${MU2EARTDAQ_EB_PORT[97]}" >> $tempFile

echo "AggregatorMain mu2edaq01-data ${MU2EARTDAQ_AG_PORT[0]}" >> $tempFile
echo "AggregatorMain mu2edaq01-data ${MU2EARTDAQ_AG_PORT[1]}" >> $tempFile

# create the logfile directories, if needed
mkdir -p -m 0777 ${logroot}/pmt
mkdir -p -m 0777 ${logroot}/masterControl
mkdir -p -m 0777 ${logroot}/boardreader
mkdir -p -m 0777 ${logroot}/eventbuilder
mkdir -p -m 0777 ${logroot}/aggregator

# start PMT
#wget -O /dev/null -q "http://mu2edaq01.fnal.gov/ganglia/api/events.php?action=add&start_time=now&summary=Mu2e Pilot System Start&host_regex=mu2edaq"
pmt.rb -p ${MU2EARTDAQ_PMT_PORT} -d $tempFile --logpath ${logroot} --display ${DISPLAY}
rm $tempFile
