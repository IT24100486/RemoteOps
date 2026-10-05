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

RemoteOps is a TCP/IP remote system monitoring and management tool consisting of an Agent (server) and a Controller (client). UDP monitoring will be added for periodic monitoring data.

The implementation follows the communication protocol specified in the IE3090 assignment brief.

## Current Implementation Status

The following TCP functionality has been implemented and tested:

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
- Graceful QUIT handling

### Allowed EXEC Commands

```text
EXEC DATE
EXEC UPTIME
EXEC DISKFREE
EXEC HOSTNAME
EXEC WHOAMI
