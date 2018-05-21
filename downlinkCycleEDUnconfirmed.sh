#!/usr/bin/env bash

# DOWNLINK Network Performances and transmissions required - loop on ED


# Useful variables
# i= number of nDevices
# increment= increment in EDs
gatewayRings=1
i=500
increment=500
maxEndDevices=6001
radius=6300
#gwrad=$(echo "10000/(2*($gatewayRings - 1)+1)" | bc -l)
globalrun=1
maxRuns=5
appPeriod=$1;

if [ $appPeriod -eq 0 ]
    then
        mixedPeriods=true
        maxEndDevices=3001
    else
        mixedPeriods=false
fi

# echo "***** DOWNLINK VARYING NUMBER OF END DEVICES *****"
# echo "            INPUT PARAMETERS    "
# echo "appPeriod= $appPeriod"
# echo "periodsToSimulate= $periodsToSimulate"
# echo "transientPeriods= $transientPeriods"
# echo "Data Rate adaptation= $DRAdapt"
# echo "max number of transmission= $maxNumbTx"
# echo "mixedPeriods= $mixedPeriods"
# echo "***************************"

# Move to the waf directory
cd ../../

echo -n "Configuring and building..."
./waf --build-profile=optimized --out=build/optimized configure > /dev/null
./waf build > /dev/null
echo " done."

# Run the script with a fixed period
while [ $i -le $maxEndDevices ]
do

    # Perform multiple runs
    currentrun=1

    while [ $currentrun -le $maxRuns ]
    do
        output="$(./waf --run "RawCompleteNetworkPerformancesWithSFUnconfirmed
            --nDevices=$i
            --gatewayRings=$gatewayRings
            --radius=$radius
            --gatewayRadius=1500
            --RngRun=$globalrun" | grep -v "build" | tr -d '\n')"
         echo "$output"

        currentrun=$(( $currentrun+1 ))
        globalrun=$(( $globalrun+1 ))
    done

    i=$(( $i+$increment ))
done
