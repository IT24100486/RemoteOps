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

The Agent provides remote system information, process listing, restricted command execution, file upload and file download functionality. UDP is used for periodic system monitoring reports.

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
- Filename/path validation
- Graceful QUIT handling
- Concurrent Controller handling using POSIX pthreads
- One worker thread per connected Controller
- Support for at least five simultaneous Controller connections
- Five-Controller concurrency testing completed successfully
- UDP monitoring using MONITOR START
- UDP monitoring stop using MONITOR STOP
- Periodic system-statistic UDP datagrams containing SID:6840
- TCP command operation during UDP monitoring
- Successful UDP monitoring stop followed by normal TCP operation
- Server-side timestamped activity logging
- Connection, authentication, command, file-transfer and monitoring event logging
- PUT and GET transfer throughput measurement in bytes per second
- File-transfer integrity verification using SHA-256 and cmp

## Concurrency Model

The RemoteOps Agent uses a thread-per-Controller concurrency model implemented with POSIX pthreads.

The main Agent thread continuously accepts incoming TCP connections. For each connected Controller, a separate worker thread is created to handle authentication, commands, file transfers, monitoring and session management. Worker threads are detached so that the main Agent can immediately continue accepting new connections.

The concurrency implementation was tested with five simultaneous Controller instances. All five Controllers successfully authenticated using the personalised token and independently processed commands while connected to the Agent on TCP port 9410.

## UDP Monitoring

UDP is used for periodic system monitoring reports.

The Controller prepares a UDP socket and sends:

MONITOR START <udp_port>

to the Agent. The Agent then sends periodic monitoring datagrams to the Controller's IP address and specified UDP port.

Example monitoring information includes CPU load, memory usage, system uptime and the personalised SID.

Monitoring is stopped using:

MONITOR STOP

The Controller was tested to confirm that UDP reports stop successfully and that normal TCP commands such as SYSINFO continue to work afterward.

## Allowed EXEC Commands

The EXEC command supports exactly the following commands:

EXEC DATE
EXEC UPTIME
EXEC DISKFREE
EXEC HOSTNAME
EXEC WHOAMI

## Personalised Storage

Uploaded files are stored under:

./agentfiles/IT24100486/

The implementation validates filenames to prevent path traversal outside the personalised storage directory.

## Development

The project is developed incrementally using Git and GitHub. Major implementation stages are committed after successful testing.

Current completed development stages include:

1. Project setup
2. Core TCP connection
3. Authentication
4. System information and command execution
5. PUT file upload
6. GET file download
7. Concurrent Controller handling
8. UDP monitoring
9. Server-side logging
10. Transfer throughput measurement
11. Final file integrity and functional testing

## Logging and Throughput

The Agent records timestamped connections, authentication results,
commands, file transfers, monitoring events and disconnects in:

remoteops_IT24100486.log

The Controller reports PUT and GET transfer throughput in bytes per
second.

File-transfer integrity was verified using SHA-256 hashes and the
cmp command. The original and downloaded files produced identical
hashes and no byte-level differences.

## Compilation

Agent:

gcc -Wall -Wextra -pthread -o agent_486 agent_486.c

Controller:

gcc -Wall -Wextra -pthread -o controller_486 controller_486.c

