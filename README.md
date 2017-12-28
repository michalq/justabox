# Bluetooth

1. Pair with ```HC-06```
2. Pin code ```1234```
3. Address ```98:D3:31:FD:7C:B6```

# BT test
```bash
# Start recording & set id=1 <1, 4> & set name 'xyz'
printf '\001\001xyz\xC8' > /dev/rfcomm0

# Send day, hour from, hour to, temperature
printf "\001\x$(printf "%x" 0)\x$(printf "%x" 11)\x$(printf "%x" 23)" > /dev/rfcomm0
printf "\001\x$(printf "%x" 12)\x$(printf "%x" 23)\x$(printf "%x" 19)" > /dev/rfcomm0

# Send end byte, crc, end byte
printf '\xC8\001\xC8' > /dev/rfcomm0

#
printf '\xC8' > /dev/rfcomm0
```