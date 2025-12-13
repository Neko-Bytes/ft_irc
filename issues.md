# Log date: 13.12.2025

## Error:

### Status = Fixed!

```
:nick!nick@localhost JOIN #test
:ircserver 353 nick = #test :nick
:ircserver 366 nick #test :End of NAMES list
:ircserver 331 nick #test :No topic is set
mode #test
:ircserver 324 nick #test +
MODE #optest +o ClientB
:ircserver 403 * #optest :No such channel
MODE #optest +o ClientB        
:ircserver 403 * #optest :No such channel
mode #test +o bob
:nick!nick@localhost MODE #test +o bob
MODE #test +o bob
:nick!nick@localhost MODE #test +o bob
:bob!bob@localhost JOIN #test
MODE #test +o bob
:nick!nick@localhost MODE #test +o bob

```

> How is mode being executed even when bob didnt exist in the channel??
> Because it is not being checked in handleMODE +o case block. [Teja] added that

## Error:

### Status = Not fixed

### Terminal 1: 

```
 ╭─teja@teja in ~ took 2m17s
[🧱] × nc localhost 6667
pass pass
nick nick
user nick * 0 : nick
:ircserver 001 nick :Welcome to the IRC server!
privmsg bob : hi
:ircserver 401 * bob :No such nick
```

### Terminal 2: 

```
 ╭─teja@teja in ~ took 1h2m8s
[🧱] × nc localhost 6667 
pass pass
nick nick
:ircserver 433 * nick :Nickname is already in use
nick bob
user nick * 0 : nick
:ircserver 001 bob :Welcome to the IRC server!
privmsg bob : hi
:bob!nick@localhost PRIVMSG bob : hi

```

> You are allowed to have same usernames but not same nicknames?

## Logs:

```
 ╰─λ val ./ircserv 6667 pass            
==255306== Memcheck, a memory error detector
==255306== Copyright (C) 2002-2024, and GNU GPL'd, by Julian Seward et al.
==255306== Using Valgrind-3.25.1 and LibVEX; rerun with -h for copyright info
==255306== Command: ./ircserv 6667 pass
==255306== 
[21:19:32] [ INFO  ] [  Server  ] Listening on port 6667
[21:19:32] [ INFO  ] [  Server  ] Password authentication enabled
[21:19:35] [CONNECT] [  Socket  ] New connection from 127.0.0.1:35878 (FD: 4)
[21:19:38] [COMMAND] [ Unknown  ] pass pass 
[21:19:41] [COMMAND] [ Unknown  ] nick nick 
[21:19:48] [COMMAND] [   nick   ] user nick * 0 : nick
[21:19:58] [COMMAND] [   nick   ] privmsg bob : hi
[21:20:04] [CONNECT] [  Socket  ] New connection from 127.0.0.1:50228 (FD: 5)
[21:20:06] [COMMAND] [ Unknown  ] pass pass 
[21:20:10] [COMMAND] [ Unknown  ] nick nick 
[21:20:13] [COMMAND] [ Unknown  ] nick bob 
[21:20:25] [COMMAND] [   bob    ] user nick * 0 : nick
[21:20:34] [COMMAND] [   bob    ] privmsg bob : hi
[21:22:14] [COMMAND] [   bob    ] join #test 
[21:22:18] [COMMAND] [   bob    ] join #test2 
[21:22:23] [COMMAND] [   bob    ] join #test3 
[21:22:33] [COMMAND] [   bob    ] privmsg #test3 : yo
[21:22:52] [COMMAND] [   nick   ] privmsg #test: can u hear me? 
[21:23:02] [COMMAND] [   nick   ] privmsg #test : can u hear me?
[21:23:18] [COMMAND] [   nick   ] privmsg #test3 : can u hear me?
[21:23:23] [COMMAND] [   nick   ] join test3 
[21:23:31] [COMMAND] [   nick   ] privmsg test3 : can u hear me
[21:23:40] [COMMAND] [   nick   ] privmsg #test3 : can u hear me
[21:23:48] [COMMAND] [   nick   ] QUIT 
[21:23:48] [DISCONN] [  Socket  ] Client (FD: 4) disconnected. Reason: Connection closed by peer or quit
[21:23:53] [COMMAND] [   bob    ] QUIT 
[21:23:53] [DISCONN] [  Socket  ] Client (FD: 5) disconnected. Reason: Connection closed by peer or quit
[21:23:57] [ INFO  ] [  Signal  ] Signal received! Initiating shutdown...
[21:23:57] [ INFO  ] [  Server  ] Server shutdown: All resources freed.
==255306== 
==255306== HEAP SUMMARY:
==255306==     in use at exit: 0 bytes in 0 blocks
==255306==   total heap usage: 312 allocs, 312 frees, 95,966 bytes allocated
==255306== 
==255306== All heap blocks were freed -- no leaks are possible
==255306== 
==255306== For lists of detected and suppressed errors, rerun with: -s
==255306== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

> So no leaks!
