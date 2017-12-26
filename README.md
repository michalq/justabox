# Bluetooth

1. Pair with ```HC-06```
2. Pin code ```1234```
3. Address ```98:D3:31:FD:7C:B6```

# BT test
```bash
# Start recording & set id=1 <1, 4> & set name 'xyz'
printf '\001\001xyz\xC8' > /dev/rfcomm0

# Send day, hour from, hour to, temperature
printf "\x$(printf "%x%x%x%x" 1 0 11 23)"
printf "\x$(printf "%x%x%x%x" 1 12 23 23)"
printf "\x$(printf "%x%x%x%x" 2 0 11 23)"
printf "\x$(printf "%x%x%x%x" 2 12 23 23)"
printf "\x$(printf "%x%x%x%x" 3 0 11 23)"
printf "\x$(printf "%x%x%x%x" 3 12 23 23)"
printf "\x$(printf "%x%x%x%x" 4 0 11 23)"
printf "\x$(printf "%x%x%x%x" 4 12 23 23)"
printf "\x$(printf "%x%x%x%x" 5 0 11 23)"
printf "\x$(printf "%x%x%x%x" 5 12 23 23)"
printf "\x$(printf "%x%x%x%x" 6 0 11 23)"
printf "\x$(printf "%x%x%x%x" 6 12 23 23)"
printf "\x$(printf "%x%x%x%x" 7 0 11 23)"
printf "\x$(printf "%x%x%x%x" 7 12 23 23)"

# Send end byte, crc, end byte
printf '\xC8\001\xC8' > /dev/rfcomm0
```