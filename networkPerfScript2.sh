#!/usr/bin/env bash

# Useful variables
# i= number of nDevices
# increment= increment in EDs
gatewayRings=1
i=1
increment=3
maxEndDevices=6
radius=7500
#gwrad=$(echo "10000/(2*($gatewayRings - 1)+1)" | bc -l)
simTime=86400
maxRuns=5
globalrun=1

# Move to the waf directory
 cd ../../

# Configure waf to use the optimized build
echo "GR:$gatewayRings, r:$radius, gwr:$gwrad, sim:$simTime, runs:$maxRuns"
echo "Warning: remember to correctly set up the following:"
echo "- Channels"
echo "- Receive paths"
echo "- Path loss"
echo -n "Configuring and building..."
./waf --build-profile=optimized --out=build/optimized configure > /dev/null
./waf build > /dev/null
echo " done."

echo "Entering external loop"

# Run the script with a fixed period
while [ $i -le $maxEndDevices ]
do
    echo -n "$i "
    # Perform multiple runs
    currentrun=1
    centraldevicessum=0
    centralreceivedsum=0
    centralreceivedAvgsum=0
    centralreceivedProbsum=0

    while [ $currentrun -le $maxRuns ]
    do
        # nGateways=$[3*$gatewayRings*$gatewayRings-$[3*$gatewayRings]+1]
        echo -n "Simulating a system with $i end devices and a transmission period of $simTime seconds...  "
        # START=$(date +%s)
        output="$(./waf --run "networkPerf2
            --nDevices=$i
            --gatewayRings=$gatewayRings 
            --radius=$radius
            --gatewayRadius=1500
            --simulationTime=$simTime
            --appPeriod=5
            --RngRun=$globalrun" | grep -v "build" | tr -d '\n')"
        echo "$output"
        centraldevices=$(echo "$output" | awk '{print $1}')     # nDevices
        simulTime=$(echo "$output" | awk '{print $2}')          # simulation time
        centralreceived=$(echo "$output" | awk '{print $3}')    # received packets
        centralreceivedAvg=$(echo "$output" | awk '{print $4}')    # received average= received packets/nDevices
        centralreceivedProb=$(echo "$output" | awk '{print $5}')    # received Prob = received packets/total sent packets

        # Sum the results
        centraldevicessum=$(echo "$centraldevicessum + $centraldevices" | bc -l)
        centralreceivedsum=$(echo "$centralreceivedsum + $centralreceived" | bc -l)
        centralreceivedAvgsum=$(echo "$centralreceivedAvgsum + $centralreceivedAvg" | bc -l)
        centralreceivedProbsum=$(echo "$centralreceivedProbsum + $centralreceivedProb" | bc -l)

        currentrun=$(( $currentrun+1 ))
        globalrun=$(( $globalrun+1 ))
    done
    
    # Average in runs
    #echo "Central averaged results centraldevicessum= $centraldevicessum , maxRuns= $maxRuns"
    echo -n " $(echo "$centraldevicessum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralreceivedsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralreceivedAvgsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralreceivedProbsum/$maxRuns" | bc -l)"


    i=$(( $i+$increment ))
done