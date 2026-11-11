# Reference audit

The operating manual required attempts to read:

1. JoeChen2me/RK-MediaProject
2. KravitzLabDevices/FED3
3. 201811510216lf/RAT-MONITOR-RELEASE branch codex/websocket-v2

On 2026-11-11, git ls-remote was attempted for all three URLs. The execution
environment returned a DNS/thread startup failure before any README, source,
CMake, include, style file, license or branch could be read. No source code from
these repositories is copied or claimed as inspected. This is BLOCKED_EXTERNAL.

The implementation therefore follows only the explicit manual requirements:
include/src separation, RAII on Host, fixed buffers at the target boundary,
explicit codec ownership, and truthful hardware status. Once network access is
available, re-run this audit and record exact commits/licenses before reusing any
algorithm or model parameter.

