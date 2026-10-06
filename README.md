# RemoteOps — IE3090 Network Programming

## Student Information

- Registration Number: IT24100486
- Agent Port: 9410
- Session ID (SID): 6840
- Authentication Token: OPS-0486
- Source Files: agent_486.c, controller_486.c
- Makefile: Makefile_486
- Log File: remoteops_IT24100486.log
- Storage Path: ./agentfiles/IT24100486/

## Project Overview

RemoteOps is a TCP/IP remote system monitoring and management tool consisting of an Agent (server) and a Controller (client).

The Agent provides remote system information, process listing, restricted command execution, file upload and file download functionality. UDP monitoring and additional management features will be implemented in later development stages.

The implementation follows the communication protocol specified in the IE3090 assignment brief.

## Current Implementation Status

The following functionality has been implemented and tested:

- TCP Agent/Controller connection
- Authentication using the personalised token
- Personalised Session ID (SID)
- Persistent TCP command session
- Line-based TCP command handling
- SYSINFO
- LISTPROC
- EXEC command whitelist
- Pre-authentication command rejection
- EXEC argument validation
- PUT file upload with exact byte-count transfer
- GET file download with exact byte-count transfer
- GET file-not-found error handling
- Personalised file storage
- Filename/path validation for uploaded files
- Graceful QUIT handling
- Concurrent Controller handling using POSIX pthreads
- One worker thread per connected Controller
- Support for at least five simultaneous Controller connections
- Five-Controller concurrency testing completed successfully

## Concurrency Model

The RemoteOps Agent uses a thread-per-Controller concurrency model implemented with POSIX pthreads.

The main Agent thread continuously accepts incoming TCP connections. For each connected Controller, a separate worker thread is created to handle authentication, commands, file transfers, and session management. Worker threads are detached so that the main Agent can immediately return to accepting new connections.

The concurrency implementation was tested with five simultaneous Controller instances. All five Controllers successfully authenticated using the personalised token and independently processed commands while connected to the Agent on TCP port 9410.

## Allowed EXEC Commands

The EXEC command supports exactly the following commands:

```text
EXEC DATE
EXEC UPTIME
EXEC DISKFREE
EXEC HOSTNAME
EXEC WHOAMI
