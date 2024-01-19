# 112_1-OS_kernel_module

this is a kernel modules ,thak can it can register a character device,an read a list of system info
1:on the user space compile the kfetch.c and kfetch.h 
2:using gcc -o kfetch kfetch.c to compile the c code
4:following the procedure:
(a):>>make
        make the module
(b):>>make load
(3):>>sudo ./kfetch -(optional)
```.c
Usage:
        ./kfetch [options]
Options:
        -a  Show all information
        -c  Show CPU model name
        -m  Show memory information
        -n  Show the number of CPU cores
        -p  Show the number of processes
        -r  Show the kernel release information
        -u  Show how long the system has been running
```
