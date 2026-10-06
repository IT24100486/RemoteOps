# IE3090 Part 1 — AI Prompt Log

This log records substantive AI interactions that materially contributed to the development of Part 1. It is not a transcript of every conversation or clarification. Entries briefly record the purpose of the interaction and how the resulting information or code was evaluated, modified, and tested.

| Date | AI Tool | Purpose / Prompt Summary | How the Output Was Used |
|---|---|---|---|
| 2026-10-05 | ChatGPT | Helped interpret the RemoteOps assignment requirements and organize the development, testing, documentation, and process evidence. | Used to plan incremental development stages and keep the implementation aligned with the assignment requirements. |
| 2026-10-05 | ChatGPT | Explained Git/GitHub project setup and HTTPS authentication for pushing the project from CentOS. | Followed the guidance, configured the repository, and verified that the initial project commit was successfully pushed to GitHub. |
| 2026-10-06 | ChatGPT | Helped implement SYSINFO, LISTPROC and the EXEC whitelist, including pre-authentication rejection and TCP response handling. | Reviewed, compiled and tested the implementation; identified and fixed acceptance of extra EXEC arguments. |
| 2026-10-06 | ChatGPT | Helped implement TCP PUT file upload in the Agent and Controller, including exact byte-count handling and personalised storage. | Reviewed the code, compiled both programs, tested a 37-byte upload, and verified the uploaded file using `cmp` and SHA-256 hashes. |

## Future entries

Only substantive AI assistance used during development will be recorded. Examples include:
- Explaining a networking concept or socket API needed for the implementation.
- Helping diagnose a specific compiler, socket, threading, or protocol error.
- Providing or explaining a small code segment that is then reviewed, adapted, and tested.
- Reviewing an implementation or test result and suggesting a correction.

Routine questions, simple confirmations, and minor conversational clarifications will not be recorded individually.
