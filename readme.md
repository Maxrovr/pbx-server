## Introduction

Implementaion a server that simulates the behavior
of a Private Branch Exchange (PBX) telephone system.

There are three modules:
  * Server initialization in `main.c`
  * Server module in `server.c`  
  * The PBX module in `pbx.c`

## Usage

`pbx -h <hostname> -p <port>`

* `-h <hostname>`
    Specify the hostname to use when connecting to the server.  The default is `localhost`.
* `-p <port #>`
    Specify the port number to use when connecting to the server.  The default is `3333`.

## Getting Started

  * From pre-built executable  
    * `demo/pbx -p 3333`[1](#[1])
  
  Or,

  * From Source Code
    * Clone the code
    * Make using `make clean` at the source directory
    * Run it by typing the following command: `bin/pbx -p 3333`[1]

  And then,

  * From another terminal window, use `telnet` to connect to the server as follows: `telnet localhost 3333`

You can now issue the following commands to the server:

```
pickup
dial <TU#>
chat <msg>
hangup
```

A simple 2 Client Client Server connection and communication looks like this (In this order):

Terminal window 1:
```
demo/pbx -p 3333
```
Terminal window 2:
```
telnet localhost 3333
```
Terminal window 3:
```
telnet localhost 3333
pickup
dial 3 // connects to telnet in terminal 2 (User File Descriptors start from 3)
```
Terminal window 2:
```
pickup // telnet in terminal 3 will be connected after the pickup
chat hello
.....
```
Terminal window 3:
```
chat hello to you too
......
```
Send hangup from either to terminate

If you make a second connection to the server from yet another terminal window,
you can make calls.

##### [1] 
You may replace `3333` by any port number `1024` or above. The server should report that it has been initialized and is listening on the specified port.port numbers below 1024 are generally reserved for use as "well-known" ports for particular services, and require "root" privilege to be used

### The Base Code

Here is the structure of the base code:

```
.
└── pbx-server
    ├── demo
    │   └── pbx
    ├── include
    │   ├── debug.h
    │   ├── pbx.h
    │   └── server.h
    ├── lib
    │   └── pbx.a
    ├── Makefile
    ├── src
    │   ├── globals.c
    │   └── main.c
    ├── tests
    │   └── .git_keep
    └── util
        └── tester.c
```


## The PBX Server: Overview and Details

The PBX system maintains the set of registered clients in the form of a mapping
from assigned extension numbers to clients.
It also maintains, for each registered client, information about the current
state of the TU for that client.  The following are the possible states of a TU:

  * **On hook**: The TU handset is on the switchhook and the TU is idle.
  * **Ringing**: The TU handset is on the switchhook and the TU ringer is
active, indicating the presence of an incoming call.
  * **Dial tone**:  The TU handset is off the switchhook and a dial tone is
being played over the TU receiver.
  * **Ring back**:  The TU handset is off the switchhook and a "ring back"
signal is being played over the TU receiver.
  * **Busy signal**:  The TU handset is off the switchhook and a "busy"
signal is being played over the TU receiver.
  * **Connected**:  The TU handset is off the switchhook and a connection has been
established between this TU and the TU at another extension.  In this state
it is possible for users at the two TUs to "chat" with each other over the
connection.
  * **Error**:  The TU handset is off the switchhook and an "error" signal
is being played over the TU receiver.

  * Commands from Client to Server
    * **pickup**
    * **hangup**
    * **dial** #, where # is the number of the extension to be dialed.
    *  **chat** ...arbitrary text...

  * Responses from Server to Client
    * **ON HOOK** #, where # reports the extension number of the client.
    * **RINGING**
    * **DIAL TONE**
    * **RING BACK**
    * **CONNECTED** #, where # is the number of the extension to which the
      connection exists.
    * **ERROR**
    * **CHAT** ...arbitrary text...