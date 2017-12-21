# Bluetooth

1. Pair with ```HC-06```
2. Pin code ```1234```
3. Address ```98:D3:31:FD:7C:B6```

# BT test
```bash
# Start recording
printf '\001' > /dev/rfcomm0

# Set id=1 <1, 4>
printf '\001' > /dev/rfcomm0

# Send name
printf 'x' > /dev/rfcomm0
printf 'y' > /dev/rfcomm0
printf 'z' > /dev/rfcomm0
printf '\xC8' > /dev/rfcomm0

# Send day
printf '\001' > /dev/rfcomm0

# Send hour from
printf "\x$(printf "%x" 0)"

# Send hour to
printf "\x$(printf "%x" 12)"

# Send temperature
printf "\x$(printf "%x" 23)"

# Send end byte
printf '\xC8' > /dev/rfcomm0

# Send CRC
printf '\001' > /dev/rfcomm0

# Send end byte
printf '\xC8' > /dev/rfcomm0
```