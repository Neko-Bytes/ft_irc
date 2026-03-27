# Building an IRC Server

---

# What is IRC?
**Internet Relay Chat** is the ancestor protocol of modern group chat like discord, slack, etc. 

* **Architecture:** Client-Server model.
* **Format:** Strictly text-based, governed by strict RFC protocols (RFC 1459, 2812).
* **Topology:** Users connect to a server, authenticate, and join `#channels` to broadcast messages or send direct messages to users.


**Note:** This server is built completely based on RFC1459 and modern.ircdocs.horse documentation

---

# IRC vs. Modern Chat (Discord/Slack)

| Feature | IRC | Discord / Slack |
| :--- | :--- | :--- |
| **Protocol** | Open (Anyone can build a client/server) | Closed / Proprietary |
| **History** | Ephemeral (Lost when you close the app) | Persistent (Saved on their databases) |
| **Media** | Only text (Images require external links) | Text, native voice, video, file transfer |

---

# The Server Setup

**Socket Configuration:**
* `AF_INET`: We explicitly bind to **IPv4**.
* `SOCK_STREAM`: We use **TCP** to guarantee that messages arrive exactly in the order they were sent.
* `SO_REUSEADDR`: A crucial socket option that lets us instantly restart the server without the OS complaining the port is "already in use."

---

# The Concurrency Problem
**The Goal:** Handle dozens of users chatting at the exact same time.

**The Naive Approach:** Spawn a new OS thread or process (`fork()`) for every single user that connects.
* *The Problem:* Eats up RAM, high CPU context switching overhead, and modifying shared channels requires complex Mutex locking to prevent crashes.

**The Solution:** Non-blocking I/O & Multiplexing.

---

# Multiplexing with poll()
Instead of multi-threading, we use `poll()`.

* **What it is:** A system call that watches an array of file descriptors (sockets) for events.
* **How it works:** It puts the server to sleep until a socket triggers an event like "Hey, I have a message to read!" (`POLLIN`) or "I have space in my buffer to send a message!" (`POLLOUT`).

**Why we use it:**
We manage all users in one single, blazing-fast loop. Because there is only one thread working on all sockets, we completely eliminate race conditions.

---

# Decoding the poll() Flags
**How the OS talks to our Server**

To manage multiple users without threads, our server passes an array of sockets to the OS. The OS then flips specific "flags" on those sockets to tell us what to do next.

* **`POLLIN` (The "Ready to Read" Flag)**
  * **On the Listener(Server) Socket:** A brand new user is knocking at the door. We need to `accept()` their connection.
  * **On a Client Socket:** The user just hit 'Enter'. There is a raw IRC message waiting in the OS buffer for us to `recv()`.

* **`POLLOUT` (The "Clear to Write" Flag)**
  * **What it means:** The network is not congested, and the OS buffer has space. We are safe to `send()` our outgoing messages without the server freezing up waiting for the network.

* **`POLLHUP` (The "Hang Up" Flag)**
  * **What it means:** The client unexpectedly severed the TCP connection (closed their laptop, lost Wi-Fi, or crashed). We must immediately kick them from all channels and free their memory.

* **`POLLERR` (The "Fatal Error" Flag)**
  * **What it means:** The socket itself is broken at the hardware or OS level. Just like a hangup, we drop the client to protect the server.

---

# The Event Loop Lifecycle
**Multiplexing in Action:** How we handle everything in one thread.

```text
       [ Start mainLoop() ]
                │
 ┌──────────────▼──────────────┐
 │ 1. Prepare pollfds          │ Set POLLIN (ready to read) or 
 │    (Update event flags)     │ POLLOUT (data waiting to send)
 └──────────────┬──────────────┘
                │
 ┌──────────────▼──────────────┐
 │ 2. poll()                   │ The server goes to sleep here.
 │    (Wait for network I/O)   │ Wakes up instantly on any event!
 └──────────────┬──────────────┘
                │
 ┌──────────────▼──────────────┐
 │ 3. Process Triggered FDs    │ Iterate through the socket array
 └──────────────┬──────────────┘
                │
                ├─► Is Listener FD? ───────► acceptNewClient()
                │
                └─► Is Client FD? 
                      │
                      ├─► Error? (POLLHUP) ─► removeClient()
                      │
                      ├─► Read?  (POLLIN)  ─► handleClientRead() ─► handleCommand()
                      │
                      └─► Write? (POLLOUT) ─► send() ─► consumeBytes()
                │
 ┌──────────────▼──────────────┐
 │        Loop Restarts        │
 └─────────────────────────────┘
```

---

# The Lifecycle of a Message
**How data flows from Client ➔ Server ➔ Client without blocking.**

```text
 ┌──────────────┐          TCP Stream          ┌───────────────────┐
 │ Sender (Netcat)├───────────────────────────►│ Server: recv()    │
 └──────────────┘                              └─────────┬─────────┘
                                                         │ 1. Read Raw Bytes
 ┌───────────────────────────────────────────────────────▼─────────┐
 │                     Client Input Buffer                         │
 │  (Accumulates fragments: "PRIV" ➔ "MSG #a" ➔ " :hi\r\n")        │
 └───────────────────────────────────────┬─────────────────────────┘
                                         │ 2. Extract on "\r\n"
 ┌───────────────────────────────────────▼─────────────────────────┐
 │                   Command Parser & Router                       │
 │  (Identifies target: Is it a #channel or a @nickname?)          │
 └─────────────┬─────────────────────────────────────┬─────────────┘
               │ 3a. Direct Message                  │ 3b. Channel Broadcast
 ┌─────────────▼─────────────┐         ┌─────────────▼─────────────┐
 │ Target Client             │         │ Channel Member List       │
 │ Output Buffer             │         │ Output Buffers (x50 users)│
 └─────────────┬─────────────┘         └─────────────┬─────────────┘
               │ 4. Wait for OS to set POLLOUT       │
 ┌─────────────▼─────────────┐         ┌─────────────▼─────────────┐
 │ Server: send()            │         │ Server: send()            │
 └─────────────┬─────────────┘         └─────────────┬─────────────┘
               │                                     │
 ┌─────────────▼─────────────┐         ┌─────────────▼─────────────┐
 │ Receiver                  │         │ Multiple Receivers        │
 └───────────────────────────┘         └───────────────────────────┘
```

---

# The Handshake: Getting Past the Bouncer
**A TCP connection does not equal an IRC connection.** When a client first connects to our port, they are placed in a restricted "unregistered" state. They cannot chat, join channels, or see other users. 

To prove they speak the protocol, they must complete the Registration Handshake:

1. `PASS secret_password` (Authenticates with the server)
2. `NICK nickname` (Claims a unique, available username)
3. `USER username * 0 :Real Name` (Provides system info)

**The Payoff:** If all three succeed, the server replies with `RPL_WELCOME (001)`. The client is now officially online!

---

# Handling the messages
**How do we translate a raw TCP string into an action?**

IRC commands follow a strictly defined structure from the 1993 RFC 1459 specification. Every message must end with `\r\n` and cannot exceed 512 bytes.

**The Anatomy:** `[:prefix] COMMAND [params...] [:trailing]`

* **Example 1:** `JOIN #general`
  * *Command:* `JOIN` | *Param 1:* `#general`
* **Example 2:** `PRIVMSG bob :Hello there, how are you?`
  * *Command:* `PRIVMSG` | *Param 1:* `bob` 
  * *Trailing:* `Hello there, how are you?` (The `:` tells our parser that spaces no longer separate parameters; everything after is one big string).

---

# Channel Architecture

* **The Global Map:** The server holds a map(array) matching a string (like `#general`) to a `Channel` object.
* **The Roster:** Inside that `Channel` object is a `std::vector<Client*>` containing pointers to every user currently in the room.
* **Broadcasting:** When someone sends a `PRIVMSG` to `#general`, the server doesn't search the whole network. It just loops through that vector and drops the message into everyone's Output Buffer (except the sender's).

---

# The JOIN Sequence
**Joining a channel triggers the following events.**

```text
 Client: "JOIN #linux"
          │
          ▼
 ┌───────────────────────────────────────┐
 │ 1. Does #linux exist?                 │
 │    NO: Create it, make user Operator. │
 │    YES: Check passwords / limits.     │
 └────────┬──────────────────────────────┘
          │ 2. Add User to Channel Vector
          ▼
 ┌───────────────────────────────────────┐
 │ 3. Broadcast to Room:                 │
 │    ":User JOIN #linux"                │
 └────────┬──────────────────────────────┘
          │ 4. Send Room Info to User
          ▼
 ┌───────────────────────────────────────┐
 │ SERVER ➔ USER: RPL_TOPIC (332)        │
 │ SERVER ➔ USER: RPL_NAMREPLY (353)     │
 │                (List of all members)  │
 └───────────────────────────────────────┘
```

---

# Operators & Modes
**Who runs the channel?**

The first person to join an empty channel is automatically granted **Operator Status** (denoted by an `@` next to their name). Operators have the power to change channel **Modes**.

Our server supports the 5 core IRC modes:
* **`+o` (Operator):** Grant or revoke admin privileges to others.
* **`+k` (Key):** Slap a password on the channel.
* **`+l` (Limit):** Cap the room at a specific number of users.
* **`+i` (Invite-Only):** Lock the doors; users must be explicitly `INVITE`d.
* **`+t` (Topic):** Restrict who is allowed to change the room's topic.

* **`-` sign is used along with the mode to remove the mode.

---

# Garbage Collection: The Empty Room
**What happens when everyone leaves?**

Channels are ephemeral. If a user types `PART #general` or unexpectedly disconnects (`QUIT`), the server removes their pointer from the channel's vector.

**The Cleanup Rule:**
If a channel's user count hits zero, the channel ceases to exist. The server immediately calls `delete` on the `Channel` object and erases it from the global map. 

* **Why?** To prevent massive memory leaks. If we kept every empty channel alive forever, trolls could crash our server by joining and leaving millions of randomly named rooms!

---

# The Hardware Monitor with Arduino
Since the monitor needs to work concurrently without the influence of poll() function, we run the display essentials on a seperate thread.

* **Background Thread:** Safely reads the counters for active users, channels, and uptime of the server.
* **USB Telemetry:** Serializes the data (`STATS:Server|Clients:5|Chans:2`) and pipes it over USB.
* **The Display:** An Arduino Uno running a custom C++ script parses the stream and draws a live UI to a 3.5" TFT LCD shield.

---

# Thread-Safe Telemetry
**How do we talk to the Arduino without slowing down the chat?**

Our server runs the main `poll()` loop on one thread, but the Arduino data is pumped out by a second, background thread (`displayThreadFunc`). 

**The Race Condition Problem:** If the main thread adds a client at the *exact millisecond* the background thread tries to read the client count, the server could crash (Data Race).

**The Lock-Free Solution:**
Instead of freezing the server with heavy Mutex locks, we used `std::atomic<int>`. 
* The main thread updates user counts using atomic Compare-And-Swap (CAS) loops.
* The background thread safely reads the snapshot once per second.
* **Result:** The hardware monitor has 0% performance impact on the chat network.

---

# Protecting the Server
**IRC is a text protocol. What happens if a malicious user connects and sends 10 Gigabytes of garbage text without ever sending a `\r\n`?**

A naive server would store it all in RAM until the computer crashes (Out Of Memory).

**Our Mitigations:**
* **Input Capping:** Each `Client` object has a hardcoded `MaxInputBufferBytes` (64KB). If you exceed it, the server silently drops your connection (`removeClient: "Input overflow"`).
* **Output Queuing:** Outbound messages are capped at 256KB to prevent a slow user from backing up the server's memory.
* **Message Truncation:** We strictly enforce the RFC 1459 rule: No message can be longer than 512 bytes.

---

# Handling abrupt client disconnects 
**The quickest way to kill a C++ server is to pull the plug on a client.**

If a user unplugs their router, their socket closes on their end, but our server might not know yet. If our server calls `send()` to push a message to that dead socket, the Linux kernel throws a fatal signal called `SIGPIPE`. 

By default, `SIGPIPE` instantly kills the entire program. 

**The Fix:** During boot, we explicitly tell the OS to ignore this signal: `signal(SIGPIPE, SIG_IGN);`. Instead of crashing the server, `send()` safely returns a `-1` error, allowing us to catch it, cleanly delete the user, and keep the server running for everyone else.

---

# Zero Leaks: Valgrind Verified
**C++ gives you total control over memory, which means you have to clean up your own mess.**

Because our server dynamically creates `Client` objects when people join, and `Channel` objects when rooms are created, memory management is critical. 

* **The Cleanup:** When a user types `QUIT` or triggers an error, we trace their pointer through every channel they joined, erase them, and run `delete`.
* **The Proof:** After stress testing with multiple users joining, chatting, parting, and dropping connections, the server was profiled with Valgrind.
* **Result:** `All heap blocks were freed -- no leaks are possible.`
---

# Thank You
### Questions?
