#! /bin/bash
# set -x
PATH_TO_TOOLS="/Users/tinmarbook/Library/Arduino15/packages/esp32/hardware/esp32/2.0.5/tools"
# PATH_TO_TOOLS="/home/tinmar/.arduino15/packages/esp32/hardware/esp32/1.0.6/tools"
curH="$(md5sum build/relaystrio.ino.bin | grep -E -o "\w+ " | sed -e 's/ //g')" # $(git rev-parse HEAD)
# serverAddr=http://lumestrio1.local:3003/knownDevices
# serverAddr=http://localhost:3003/knownDevices

missing=""
uptodate=""
updateds=""
failed=""

if [ "$1" == "" ]; then
    tryAll=1
elif [ "$1" == "f" ]; then
    force=1
    shift
    if [[ $1 == "" ]]; then
        tryAll=1
    fi
fi

# if [[ $tryAll == "1" ]]; then
# sudo dscacheutil -flushcache &&     sudo killall -HUP mDNSResponder
# fi

function dbg() {
    echo $1 >&2
}

function getAllHostNamesOnNetwork() {
    dns-sd -B _rspstrio._udp >/tmp/strios &
    sleep 1 && kill %1
    rhns=$(grep -o -E "relay_.*" /tmp/strios)
    hns=""
    for hn in $rhns; do
        hns="$hns $hn.local"
    done
    echo $hns
}

function getIpFromHostName() {
    echo $(ping -c 1 $1 | grep from | grep -E -o "\d+\.\d+\.\d+\.\d+")
}

# function getAllIpsOnNetwork() {
#     RES=""
#     for hn in $(getAllHostNamesOnNetwork); do
#         ip=$(getIpFromHostName $hn)
#         RES="$RES $ip"
#     done
#     echo $RES
# }

function getVersion() {
    v=$(curl --connect-timeout 5 $1:3000/version --silent)
    if [ $? != "0" ]; then
        echo "-1"
    else
        echo $v
    fi
}

function getRemoteSketchMd5() {
    v=$(curl --connect-timeout 5 "http://$1:3000/sketch.md5" --silent)
    if [ $? != "0" ]; then
        echo "-1"
    else
        echo $v
    fi
}

function updateIfNeeded() {
    r=$(sed -e 's/"//g' <<<"$1")
    dbg "checking $r"
    if [ "$2" != "" ]; then
        v="force"
    else
        v=$(getRemoteSketchMd5 "$r")
    fi
    if [[ "$v" == "-1" ]]; then
        dbg "$r not connected"
        missing+="$r "
        echo 1
        return
    fi
    if [ "${curH}" == "$v" ]; then
        dbg "already updated"
        uptodate+="$r "
        echo 2
    else
        dbg "$r need update was ${v}, wants ${curH},"
        upload $r
        if [ "$?" != "0" ]; then
            failed+="$r "
            echo 3
        else
            updateds+="$r "
            echo 4
        fi
    fi
}

function upload() {
    python3 "$PATH_TO_TOOLS/espota.py" --timeout 10 --progress -i "$1" -p 3232 --auth= -f build/relaystrio.ino.bin
}

function getAllKnown() {
    echo $(curl $serverAddr --silent | jq -r " to_entries[] |  select(.key | startswith(\"relay@\") )  | .value")
}
function showKd() {
    echo $(jq -r " {ip, deviceName} " <<<$1)
}
function getAllIpRegistered() {
    echo $(jq -r .ip <<<$1)
}

function getNiceNameFromIp() {
    ip=$(sed -e 's/"//g' <<<"$2")
    echo $(jq -r " select(.ip==\"$ip\") | {ip, deviceName, niceName}" <<<$1)
}

function printNicenames() {
    for a in $2; do
        echo $(getNiceNameFromIp "$1" "$a")
    done
}

function updateRegisteredInServer() {
    kd="$(getAllKnown)"
    # echo "$(showKd "$kd")"
    ips=$(getAllIpRegistered "$kd")
    dbg "$(getNiceNameFromIp "$kd" 192.168.43.74)"
    dbg "$(printNicenames "$kd" "$ips")"
    # echo $ips
    for i in $ips; do
        # echo $i
        updateIfNeeded $i $force
    done

    dbg "up to date were :\n$(printNicenames "$kd" "$uptodate") "
    dbg "successfully updated were :\n$(printNicenames "$kd" "$updateds") "
    dbg "missing were :\n$(printNicenames "$kd" "$missing")   "
    dbg "failed update were :\n$(printNicenames "$kd" "$failed") "
}

function getResForUpload() {
    ip=$(getIpFromHostName $1)
    dbg "hostname $1 is $ip"
    updateRes="3"
    if [[ $ip ]]; then
        updateRes=$(updateIfNeeded "$ip" "$force")
    else
        dbg "!!! not found ip for $1"
    fi
    echo $updateRes
}

function updateDiscovered() {
    hns=$(getAllHostNamesOnNetwork)
    missing_hns=""
    uptodate_hns=""
    failed_hns=""
    updated_hns=""
    if [[ "$hns" == "" ]]; then
        dbg "no hostnames found"
        exit 1
    fi
    echo "checking $hns"
    hns2=$hns
    resCodes=""
    . $(which env_parallel.bash)
    # parset resCodes --progress -j50 $0 ::: $hns
    parallel --progress -j50 $0 ::: $hns
    # for updateRes in $resCodes; do
    echo "got res $resCodes"
    echo "got hns $hns"
    i=0
    for hn in $hns2; do
        # echo $i
        updateRes="${resCodes[i]}"
        i=$i+1
        # updateRes=$(getResForUpload $hn)

        case $updateRes in
        "1")
            missing_hns="$missing_hns $hn"
            ;;

        "2")
            uptodate_hns="$uptodate_hns $hn"
            ;;

        "3")
            failed_hns="$failed_hns $hn"
            ;;
        "4")
            updated_hns="$updated_hns $hn"
            ;;

        *)
            exit 44
            ;;
        esac
    done

}

if [ "$tryAll" == "1" ]; then

    updateDiscovered
    echo "up to date were :\n $uptodate_hns "
    echo "successfully updated were :\n$updated_hns"
    echo "missing were :\n $missing_hns "
    echo "failed update were :\n $failed_hns"

    # updateIfNeeded "relay_1.local"
else

    for ADDR in "$@"; do
        if [[ ${#ADDR} -lt 3 ]]; then
            ADDR="relay_$ADDR.local"
        fi
        res=$(getResForUpload $ADDR)
        # ip=$(getIpFromHostName $ADDR)
        # if [[ $ip ]]; then
        #     updateIfNeeded $ip $force
        # else
        #     echo "no ip found for $ADDR"
        # fi

    done

    # getSketchMd5 $1
fi

exit 0
