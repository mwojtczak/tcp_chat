First project for Computer Networks Classes.

The main ami was to create proggram for communication between users (chat) using TCP protocol.
Client should read text from stdin and should send every line to server. At the same time it should read every line sent to him by server and write it to stdout.
Server should read messages from every cliend and send it to every connected client accept sender.
Communication between clients and server is via TCP.

Messeges send between server and clients should be build as it follows:

Number 
1. 16-bit number without sign, transmitted in  network byte order, not bigget than 1000 - length of sent text
2. Message - one line od stdin from client, without ending character 0 and without characters moving the cursor to next line. 


Usage:


server: ./server [port_number]

client: ./client host [port_number]



Default port number: 20160