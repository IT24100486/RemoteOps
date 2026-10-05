# RemoteOps Design Diary

**Student:** IT24100486  
**Module:** IE3090 Network Programming

## 2026-10-05 — Project Initialization

### Initial decisions
- Created a dedicated RemoteOps project directory in the CentOS environment.
- Used the personalised source filenames required by the assignment: `agent_486.c`, `controller_486.c`, and `Makefile_486`.
- Initialized a Git repository and connected it to the GitHub repository.
- The implementation will be developed incrementally, with each major working stage tested before moving to the next stage.
- The communication protocol provided in the assignment will be followed rather than creating a different protocol.

### Personalisation
- Agent listening port: 9410
- SID: 6840
- Authentication token: OPS-0486
- Log file: remoteops_IT24100486.log
- Storage path: ./agentfiles/IT24100486/

### Initial obstacle
GitHub HTTPS authentication did not accept the normal account password. A GitHub Personal Access Token was created and used successfully to push the initial project structure.

### Next step
Implement and test the basic TCP Agent/Controller connection before adding the remaining protocol features.

## 2026-10-06 — Core TCP Command Implementation

### Implemented features
- Added SYSINFO to return Linux load average, used memory, and system uptime.
- Added LISTPROC using a process snapshot from the Linux process list.
- Added EXEC with a strict whitelist containing DATE, UPTIME, DISKFREE, HOSTNAME, and WHOAMI.
- Added rejection of commands that are attempted before authentication.
- Added validation to reject EXEC commands containing additional arguments.
- Improved the Controller connection message to display the Agent IP address and TCP port.

### Protocol and networking decisions
- Continued using newline-based TCP command framing through recv_line().
- Added send_all() so responses are not dependent on a single successful send() call.
- Every TCP response continues to include the personalised SID:6840 tag.
- EXEC does not allow arbitrary shell commands; only the five specified commands are accepted.

### Testing
The core command set was tested through one persistent Controller session. The tests included pre-authentication command rejection, successful authentication, all five allowed EXEC commands, rejection of an unsupported EXEC command, rejection of an EXEC command with an extra argument, and graceful QUIT.

### Result
The SYSINFO, LISTPROC, EXEC and authentication-protection functionality is working with the personalised protocol values for IT24100486.

### Next step
Implement TCP file upload using the PUT protocol with exact byte-count handling and personalised file storage.
