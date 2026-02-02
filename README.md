*This project has been created as part of the 42 curriculum by kmummadi, qhahn*
# irc serv
  
## Description
 
This repo contains a non-blocking internet relay chat server, compatible with [irssi](https://irssi.org/) and [netcat](https://nc110.sourceforge.io/). it receives and relays messages and commands over TCP, thus allowing for group and direct communication between clients. functionally, this is achieved through sockets, poll, and various datastructures for the specific aspects of these operations.

## Instructions
 
### prerequisites
- ```c++17``` capable compiler
- make
- network permission to bind to ports for TCP

### Build

clone this repo and compile the server in project root directory:
```
make
```

### Running the sever

run the built executable with port and password:
```
./ircserv [port] [password]
```
6667 is the standard unencrypted port for the irc protocol.

after the server successfully started, you can now connect using [irssi](https://irssi.org/New-users/) or netcat(nc). 

### Connect with irssi
in a new terminal, run:
```
irssi
```
and then connect using irssi:
```
/connect localhost [port] [password]
```
### Connect with netcat
```
nc localhost [port]
```
then authenticate manually:
```
PASS [password]
```
```
NICK [nickname]
```
```
USER [username] 0 * :[realname]
```

### Cleaning
to clean the object files but keep the executable, use:
```
make clean
```
to also clean the executable, use:
```
make fclean
```
## Supported commands
- Authentication: ```PASS, NICK, USER, QUIT```
- Channel Operations: ```JOIN, PART, TOPIC, INVITE, KICK, MODE```
- Messaging: ```PRIVMSG, NOTICE```
- Server Queries: ```PING/PONG, WHO, WHOIS```
## Supported channel modes
- ```k``` add or remove a channel access key
- ```o``` add or remove a user from operator status
- ```i``` set the channel to invite only.
- ```l``` limit the maximum number of channel members
- ```t``` set channel topic protection status
### Limits
- ```512``` bytes per message, including ```\r\n``` terminator and commands/syntax.
- limited by file descriptor limit of host os (```ulimit -n```)
- ```NICK``` can't exceed 32 characters
- ```USER``` can't exceed 16 characters and will be truncated
## Resources
- [modern ircdocs horse(the Bible)](https://modern.ircdocs.horse)
- [RFC 2813](https://www.rfc-editor.org/rfc/rfc2813.html)
- [RFC 2812](https://www.rfc-editor.org/rfc/rfc2812.html)
- [RFC 1459](https://www.ietf.org/rfc/rfc1459.txt)
- [irc definitions](https://defs.ircdocs.horse/)

### AI Usage
AI tools were used for:
* generating boilerplate code
* debugging specific C++ errors
* testing integration of new units
* explaining some IRC protocol numerics
* refactoring some logic flow in client and channel
* Generating comments in the code for documentation purposes
