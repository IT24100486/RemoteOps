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
