#! /bin/bash -e
# set -e -x
# Hacked together by Thorsten von Eicken in 2019
# XTENSA_GDB="~/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-gdb"
XTENSA_GDB="/Users/tinmarbook/Library/Arduino15/packages/esp32/tools/xtensa-esp32-elf-gcc/gcc8_4_0-esp-2021r2-patch3/bin/xtensa-esp32-elf-gdb"

# # Validate commandline arguments
# if [ -z "$1" ]; then
#     echo "usage: $0 <elf file> [<backtrace-text>]" 1>&2
#     echo "reads from stdin if no backtrace-text is specified" 2>&2
#     exit 1
# fi

elf="/Users/tinmarbook/Dev/momo/relaystrio/build/relaystrio.ino.elf"
if ! [ -f $elf ]; then
    echo "ELF file not found ($elf)" 1>&2
    exit 1
fi

if [ -z "$1" ]; then
    echo "reading backtrace from stdin"
    bt=/dev/stdin
elif ! [ -f "$1" ]; then
    echo "Backtrace file not found ($1)" 1>&2
    exit 1
else
    bt=$1
fi

# Parse exception info and command backtrace
rePC="PC.*:? (0x40[0-2][0-9a-f]*).*"
# rePC='PC\s*:? ([^\s]*).*'
# rePC='PC.*:? ([^\s]*)'
reEA='EXCVADDR.*:? (0x40[0-2][0-9a-f]*).*'
reBT='Backtrace:(.*)'
reIN='^[0-9a-f:x ]+$'
reOT='[^0-9a-zA-Z](0x40[0-2][0-9a-f]{5})[^0-9a-zA-Z]'
inBT=0
declare -a REGS

tst='.*'
if [[ "PC      : 0x400d3c29  PS      : 0x00060a30  A0      : 0x800d687d  A1      : 0x3ffb2190" =~ $rePC ]]; then echo "ok"; else echo "ko"; fi
echo ${BASH_REMATCH[1]}
# exit
BT=
echo "reading"
while read p; do
    echo "line $p"
    if [[ "$p" =~ $rePC ]]; then
        echo "pcf ${BASH_REMATCH[1]}"
        # PC      : 0x400d38fe  PS      : 0x00060630  A0      : 0x800d35a6  A1      : 0x3ffb1c50
        REGS+=(PC:${BASH_REMATCH[1]})
    elif [[ "$p" =~ $reEA ]]; then
        echo "evf ${BASH_REMATCH[1]}"
        # EXCVADDR: 0x16000001  LBEG    : 0x400014fd  LEND    : 0x4000150d  LCOUNT  : 0xffffffff
        REGS+=(EXCVADDR:${BASH_REMATCH[1]})
    elif [[ "$p" =~ $reBT ]]; then
        # Backtrace: 0x400d38fe:0x3ffb1c50 0x400d35a3:0x3ffb1c90 0x400e40bf:0x3ffb1fb0
        BT="${BASH_REMATCH[1]}"
        echo "btf ${BASH_REMATCH[1]}"
        inBT=1
    elif [[ $inBT ]]; then
        if [[ "$p" =~ $reIN ]]; then
            # backtrace continuation lines cut by terminal emulator
            # 0x400d38fe:0x3ffb1c50 0x400d35a3:0x3ffb1c90
            BT="$BT${BASH_REMATCH[0]}"
        else
            inBT=0
        fi
    elif [[ "$p" =~ $reOT ]]; then
        # other lines with addresses
        # A2      : 0x00000001  A3      : 0x00000000  A4      : 0x16000001  A5      : 0x31a07a28
        REGS+=(OTHER:${BASH_REMATCH[1]})
    fi
done <$bt

echo "REGS is ${REGS}"
echo "BT is ${BT}"

# Parse addresses in backtrace and add them to REGS
n=0
for stk in $BT; do
    echo "line $stk"
    # ex: 0x400d38fe:0x3ffb1c50
    if [[ $stk =~ '(0x40[0-2][0-9a-f]+):?.*' ]]; then
        addr=${BASH_REMATCH[1]}
        REGS+=("BT-${n}:${addr}")
    fi
    let "n=$n + 1"
    #[[ $n -gt 10 ]] && break
done
echo "REGS2 is ${REGS}"

# REGS=(0x400d3c29 0x40084911 0x40084919 0x400d3c26 0x400d687a 0x400e8c06)
# Iterate through all addresses and ask GDB to print source info for each one
for reg in "${REGS[@]}"; do
    echo "processing $reg"
    name=${reg%:*}
    addr=${reg#*:}
    #echo "Checking $name: $addr"
    info=$($XTENSA_GDB --batch $elf -ex "set listsize 1" -ex "l *$addr" -ex q 2>&1 |
        sed -e 's;/[^ ]*/ESP32/;;' |
        egrep -v "(No such file or directory)|(^$)") || true
    if [[ -n "$info" ]]; then echo "$name: $info"; fi
done
