# set -x
PATH_TO_TOOLS="/Users/tinmarbook/Library/Arduino15/packages/esp32/hardware/esp32/2.0.4/tools"
# PATH_TO_TOOLS="/home/tinmar/.arduino15/packages/esp32/hardware/esp32/1.0.6/tools"
curH=$(git rev-parse HEAD)
serverAddr=http://lumestrio100.local:3003/knownDevices
# serverAddr=http://localhost:3003/knownDevices
missing=""
uptodate=""
updateds=""
failed=""

function getVersion() {
    v=$(curl --connect-timeout 5 $1:3000/version --silent)
    if [ $? != "0" ]; then
        echo "-1"
    else
        echo $v
    fi
}

function updateIfNeeded() {
    r=$(sed -e 's/"//g' <<<"$1")
    echo "checking $r"
    v=$(getVersion "$r")
    if [ "$v" == "-1" ]; then
        echo "$r not connected"
        missing+="$r "
        return 1
    fi
    if [ "${curH}" == "$v" ]; then
        echo "already updated"
        uptodate+="$r "
    else
        echo "need update was ${v}, wants ${curH}"
        upload $r
        if [ "$?" != "0" ]; then
            failed+="$r "
        else
            updateds+="$r "
        fi
    fi
}

function upload() {
    python3 "$PATH_TO_TOOLS/espota.py" -i "$1" -p 3232 --auth= -f build/relaystrio.ino.bin
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

if [ "$1" == "" ]; then
    kd="$(getAllKnown)"
    # echo "$(showKd "$kd")"
    ips=$(getAllIpRegistered "$kd")
    echo "$(getNiceNameFromIp "$kd" 192.168.43.74)"
    echo "$(printNicenames "$kd" "$ips")"
    # echo $ips
    for i in $ips; do
        # echo $i
        updateIfNeeded $i
    done

    echo "up to date were :\n$(printNicenames "$kd" "$uptodate") "
    echo "successfully updated were :\n$(printNicenames "$kd" "$updateds") "
    echo "missing were :\n$(printNicenames "$kd" "$missing")   "
    echo "failed update were :\n$(printNicenames "$kd" "$failed") "

    # updateIfNeeded "relay_1.local"
else
    upload $1
fi
