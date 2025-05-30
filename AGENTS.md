# AGENTS.md
## PHP Source Testing Agent Setup

This document outlines the requirements and steps for Codex agents tasked with compiling the PHP interpreter from source and running `.phpt` tests.


## Purpose

Codex agents must be capable of:

- Compiling the PHP interpreter from source
- Running `.phpt` tests using `make test`
- Reporting test results for evaluation or analysis

## Configure Build &  Compile & Run Tests
```shell
docker run --rm -it -v "$PWD":/usr/src/php -w /usr/src/php 1and1internet/php-build-environment:8.2 /usr/src/php/run.sh
```

## Output Expectations
The agent should capture and expose the .phpt test results (including failures) in a human-readable format, or as structured logs for Codex to analyze.
Compilation and tests happen in-place

## Security
Ensure no untrusted .phpt tests are executed without review. Tests can execute arbitrary code as part of interpreter testing.
