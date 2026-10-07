# RemoteOps Design Diary

**Student:** IT24100486  
**Module:** IE3090 Network Programming

## 2026-10-05 — Project Setup

Created the RemoteOps project in the CentOS environment using the personalised files `agent_486.c`, `controller_486.c`, and `Makefile_486`. Git/GitHub was configured for incremental development. The assignment protocol was followed rather than creating a custom command format.

Personalised configuration:
- TCP port: 9410
- SID: 6840
- Authentication token: OPS-0486
- Log file: remoteops_IT24100486.log
- Storage: ./agentfiles/IT24100486/

GitHub HTTPS authentication initially required a Personal Access Token instead of the normal account password.

## 2026-10-06 — TCP Commands and File Transfer

Implemented and tested the core TCP functionality:
- Authentication and pre-authentication rejection
- SYSINFO and LISTPROC
- EXEC whitelist: DATE, UPTIME, DISKFREE, HOSTNAME and WHOAMI
- EXEC argument validation
- PUT and GET file transfer with exact byte-count handling
- Personalised file storage and filename validation
- GET FILE_NOT_FOUND handling

TCP communication uses newline-based command framing, `recv_line()` for complete commands and `send_all()` for reliable transmission. File integrity was verified using `cmp` and SHA-256 hashes.

## 2026-10-07 — Concurrent Controllers and UDP Monitoring

The Agent was changed to a POSIX pthread thread-per-Controller design to satisfy the requirement for at least five simultaneous Controller connections. Five Controllers were tested successfully, with `ss` used to verify the established TCP connections.

UDP monitoring was then implemented using a separate monitoring thread on the Agent and a UDP receiver thread on the Controller. `MONITOR START <udp_port>` starts periodic system-statistic datagrams containing the personalised SID, while `MONITOR STOP` stops the monitoring stream. The Controller was also tested with normal TCP commands while UDP monitoring was active.

During testing, `MONITOR STOP` initially did not behave correctly. The implementation was reviewed and debugged with AI assistance, including Claude, and the Controller/Agent interaction was corrected. Final testing confirmed that UDP reports stop successfully, TCP SYSINFO continues to work after stopping monitoring, and QUIT closes the session correctly.

## 2026-10-07 — Logging, Throughput and Final Testing

Implemented timestamped server-side logging and added PUT/GET transfer throughput as the selected optional extension. A 37-byte file was transferred successfully, with throughput reported in bytes/second. File integrity was verified using matching SHA-256 hashes and `cmp`.

Final testing confirmed the mandatory TCP, file-transfer, concurrency, UDP monitoring and graceful disconnect features were working as intended.

## Current Design

RemoteOps uses a TCP Agent/Controller architecture. The Agent accepts multiple Controllers concurrently using POSIX pthreads. TCP is used for authentication, commands and file transfer, while UDP is used for periodic monitoring reports. The implementation follows the assignment protocol and uses the personalised port, SID, authentication token and storage path throughout.

**Current result: PASS — mandatory RemoteOps functionality and the selected throughput extension have been implemented and tested.**
