# Instructions
 1. Set necessary variables in weather-station/Makefile or export them in bash
 2. git submodule update --init # (Only needed once)
 3. make clean all -C libopencm3 # (Only needed once)
 4. make clean all -C weather-station
 5. make flash -C weather-station # To flash the binary

