#!/usr/bin/env bash

# Useful variables
# i= number of nDevices
# increment= increment in EDs
gatewayRings=1
appPeriod=10801
increment=3600
maxAppPeriod=86400 # 15 min
radius=7500
#gwrad=$(echo "10000/(2*($gatewayRings - 1)+1)" | bc -l)
# simTime=7201    simTime = appPeriod
nDevices=2000
maxRuns=5
globalrun=1


# Move to the waf directory
 cd ../../

# Configure waf to use the optimized build
# echo "GR:$gatewayRings, r:$radius, gwr:$gwrad, sim:$simTime, runs:$maxRuns"
# echo "Warning: remember to correctly set up the following:"
# echo "- Channels"
# echo "- Receive paths"
# echo "- Path loss"
# echo -n "Configuring and building..."
./waf --build-profile=optimized --out=build/optimized configure > /dev/null
./waf build > /dev/null
# echo " done."

# Run the script with a fixed period
while [ $appPeriod -le $maxAppPeriod ]
do
    
    # Perform multiple runs
    currentrun=1
    centraltimesum=0
    centralreceivedsum=0
    centralinterferedsum=0
    centralnomorerxsum=0
    centralundersenssum=0
    centraltotpacketsum=0

    while [ $currentrun -le $maxRuns ]
    do
        # nGateways=$[3*$gatewayRings*$gatewayRings-$[3*$gatewayRings]+1]
        # echo -n "Simulating a system with $i end devices and a transmission period of $simTime seconds...  "
        # START=$(date +%s)
        output="$(./waf --run "networkPerfTime
            --nDevices=$nDevices
            --gatewayRings=$gatewayRings 
            --radius=$radius
            --gatewayRadius=1500
            --simulationTime=$appPeriod
            --appPeriod=$appPeriod 
            --RngRun=$globalrun" | grep -v "build" | tr -d '\n')"
        # echo "$output"
        centraltime=$(echo "$output" | awk '{print $1}')             # nDevices
        centraltotpacket=$(echo "$output" | awk '{print $2}')             # nDevices
        centralreceived=$(echo "$output" | awk '{print $3}')            # received packets
        centralinterfered=$(echo "$output" | awk '{print $4}')          # interfered packets
        centralnomorerx=$(echo "$output" | awk '{print $5}')            # packets discarded because no more receivers available
        centralundersens=$(echo "$output" | awk '{print $6}')           # packets discarded because under sensitivity

        # Sum the results
        centraltimesum=$(echo "$centraltimesum + $centraltime" | bc -l)
        centraltotpacketsum=$(echo "$centraltotpacketsum + $centraltotpacket" | bc -l)
        centralreceivedsum=$(echo "$centralreceivedsum + $centralreceived" | bc -l)
        centralinterferedsum=$(echo "$centralinterferedsum + $centralinterfered" | bc -l)
        centralnomorerxsum=$(echo "$centralnomorerxsum + $centralnomorerx" | bc -l)
        centralundersenssum=$(echo "$centralundersenssum + $centralundersens" | bc -l)

        currentrun=$(( $currentrun+1 ))
        globalrun=$(( $globalrun+1 ))
    done
    
    # Average in runs
    #echo "Central averaged results centraldevicessum= $centraldevicessum , maxRuns= $maxRuns"
    echo -n " $(echo "$centraltimesum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centraltotpacketsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralreceivedsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralinterferedsum/$maxRuns" | bc -l)"
    echo -n " $(echo "$centralnomorerxsum/$maxRuns" | bc -l)"
    echo " $(echo "$centralundersenssum/$maxRuns" | bc -l)"


    appPeriod=$(( $appPeriod+$increment ))
done