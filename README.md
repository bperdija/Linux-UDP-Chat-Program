# Linux-UDP-Chat-Program

Thanks for popping by!

The goal of this project was to create a simple "chat"-like facility that enables a user at one terminal to communicate with a user at another terminal. The main two concepts explored in this project are Threads (creating a threads list and assigning tasks to them), and the UDP protocol (uncluding uses of socket, bind, sendto, recvfrom, getaddrinfo among others).

### To run this program, either:
a) 
- Run "make run1" in one terminal
- Run "make run2" in another terminal
- Connection is now binded. To check status of the other user, type in "!status". To exit communication, type in "!exit". These are case sensitive. 

b) 
- Run "make" 
- Run "./lets-talk <local port> <remote host> <remote port>
--> Example: "./lets-talk <3000> <192.168.0.513> <3001>"
- Run the same above command, with swapped <local port> and <remote port> to begin communication

### Sample:
https://user-images.githubusercontent.com/59851304/214943900-c8475beb-1890-4375-9ad8-4b09bcfc53ab.mov

