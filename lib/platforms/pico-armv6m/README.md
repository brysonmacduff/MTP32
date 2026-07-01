# MTP32 Library For Raspberry Pi Pico W

## Install Instructions

The headers and library file will appear in the pico platform directory where you run the installation command.

```
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=.
cmake --build build --target install
```
