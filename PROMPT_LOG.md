# IE3090 Part 1 — AI Prompt Log

This log records substantive AI interactions that materially contributed to the development of Part 1. It is not a transcript of every conversation or clarification. Entries briefly record the purpose of the interaction and how the resulting information or code was evaluated, modified, and tested.

| Date | AI Tool | Purpose / Prompt Summary | How the Output Was Used |
|---|---|---|---|
| 2026-10-05 | ChatGPT | Helped interpret the RemoteOps assignment requirements and organize the development, testing, documentation, and process evidence. | Used to plan incremental development stages and keep the implementation aligned with the assignment requirements. |
| 2026-10-05 | ChatGPT | Explained Git/GitHub project setup and HTTPS authentication for pushing the project from CentOS. | Followed the guidance, configured the repository, and verified that the initial project commit was successfully pushed to GitHub. |
| 2026-10-06 | ChatGPT | Helped implement SYSINFO, LISTPROC and the EXEC whitelist, including pre-authentication rejection and TCP response handling. | Reviewed, compiled and tested the implementation; identified and fixed acceptance of extra EXEC arguments. |
| 2026-10-06 | ChatGPT | Helped implement TCP PUT file upload in the Agent and Controller, including exact byte-count handling and personalised storage. | Reviewed the code, compiled both programs, tested a 37-byte upload, and verified the uploaded file using `cmp` and SHA-256 hashes. |
| 2026-10-06 | ChatGPT | Helped implement TCP GET file download in the Agent and Controller, including exact byte-count handling and file-not-found handling. | Reviewed the code, compiled both programs with `-Wall -Wextra`, tested a 37-byte download, verified the downloaded file using `cmp` and SHA-256, and tested the missing-file error response. |
| 2026-10-07 | ChatGPT | Helped implement concurrent Controller handling using POSIX pthreads and a thread-per-Controller design to satisfy the requirement for at least five simultaneous Controller connections. | Reviewed and adapted the implementation, compiled it successfully, tested the existing functionality with one Controller, then tested five simultaneous Controllers. All five authenticated successfully and independently processed commands; `ss` was used to verify five established TCP connections. |
| 2026-10-07 | ChatGPT | Helped implement UDP monitoring using `MONITOR START <udp_port>` and `MONITOR STOP`, including Agent monitoring and Controller UDP receiver threads. | Reviewed and adapted the implementation, compiled both programs successfully, tested periodic UDP system-statistic datagrams, verified the personalised SID, and tested TCP commands while monitoring was active. |
| 2026-10-07 | Claude | Helped diagnose the `MONITOR STOP` problem during UDP monitoring testing. | Used the debugging suggestions to inspect the Controller/Agent interaction and identify the cause of the stop-handling issue. The implementation was corrected and then retested successfully. |
| 2026-10-07 | ChatGPT | Helped review the final UDP implementation and verify the complete START → UDP reports → STOP → TCP command → QUIT workflow. | Performed a final functional test. UDP reports stopped after `MONITOR STOP`, `SYSINFO` continued to work, and the session ended successfully with `QUIT`. |
| 2026-10-07 | ChatGPT | Helped implement server-side timestamped logging for connections, authentication, commands, file transfers, monitoring and disconnects. | Reviewed the changes, compiled the Agent successfully, and verified the personalised log output. |
| 2026-10-07 | ChatGPT | Helped implement the optional transfer-throughput measurement for PUT and GET operations. | Added timing to the Controller, compiled successfully, and tested throughput reporting in bytes per second. |
| 2026-10-07 | ChatGPT | Helped verify file-transfer integrity after the throughput implementation. | Compared the original and downloaded files using SHA-256 and `cmp`; both confirmed byte-for-byte equality. |
| 2026-10-07 | ChatGPT | Reviewed the completed implementation against the assignment requirements and planned the final testing and submission evidence. | Confirmed that the mandatory features and selected throughput extension were complete and identified the remaining documentation, report, reflection and packaging tasks. |

## Future entries

Only substantive AI assistance used during development will be recorded. Examples include:
- Explaining a networking concept or socket API needed for the implementation.
- Helping diagnose a specific compiler, socket, threading, or protocol error.
- Providing or explaining a small code segment that is then reviewed, adapted, and tested.
- Reviewing an implementation or test result and suggesting a correction.

Routine questions, simple confirmations, and minor conversational clarifications will not be recorded individually.
