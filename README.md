# AVR.ENC28J60.TCP.IP
A TCP/IP stack written in C for AVR microcontroller (ATmega64 in this demo project) using ENC28J60
Driver for ENC and TCP/IP stack are written in seperated header files. So one may use only the driver or the stack
This is a demo project where the controller is in server mode and it transmits the data caming through TCP/IP and wrap the incoming serial data into a TCP packet and send it through the ENC when it is connected
ACK and SCK numbers are managed in the library. So, the user do not need to worry about them.
Enjoy :)
