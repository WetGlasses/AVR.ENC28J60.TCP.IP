# AVR.ENC28J60.TCP.IP.UDP
A TCP/IP stack written in C for AVR microcontroller (ATmega64 in this demo project) using ENC28J60.
<br />UDP protocol is also implemented.
<br />Driver for ENC and TCP/IP stack are written in seperated header files. So one may use only the driver or the stack.
<br />This is a demo project where the controller is in server mode and it transmits the data caming through TCP/IP and wrap the incoming serial data into a TCP packet and send it through the ENC when it is connected.
<br />ACK and SCK numbers are managed in the library. So, the user do not need to worry about them.
<br />Settings are changed by sending UDP packets.
<br />AVR Studio 7 is used in the development
<br />Enjoy :)
