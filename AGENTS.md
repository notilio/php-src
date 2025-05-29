# AGENTS.md
## PHP Source Testing Agent Setup

This document outlines the requirements and steps for Codex agents tasked with compiling the PHP interpreter from source and running `.phpt` tests.


## Purpose

Codex agents must be capable of:

- Compiling the PHP interpreter from source
- Running `.phpt` tests using `make test`
- Reporting test results for evaluation or analysis

## Configure Build
```shell
./buildconf --force
./configure \
    --prefix=/usr/local/php-dev \
    --enable-debug \
    --enable-mbstring \
    --enable-zip \
    --enable-soap \
    --enable-intl \
    --enable-opcache \
    --enable-sockets \
    --enable-pcntl \
    --with-curl \
    --with-openssl \
    --with-zlib \
    --with-pdo-mysql \
    --with-pdo-pgsql \
    --with-mysqli \
    --with-xsl \
    --with-gmp \
    --with-readline \
    --with-password-argon2 \
    --with-ffi \
    --with-jpeg \
    --with-webp \
    --with-freetype
```
> *NOTE!*: Run once before first compilation

## Compile
```shell
make -j$(nproc)
```

## Install (Local Only)
```
make install
```

## Add to PATH
```shell
export PATH=/usr/local/php-dev/bin:$PATH
```

## Run Tests
```shell
make test
```

Use TESTS=ext/mbstring/tests to limit the test scope if needed.

## Output Expectations
The agent should capture and expose the .phpt test results (including failures) in a human-readable format, or as structured logs for Codex to analyze.
Compilation and tests happen in-place

## Tips
Clear previous build artifacts (make clean) between test runs if needed.

## Security
Ensure no untrusted .phpt tests are executed without review. Tests can execute arbitrary code as part of interpreter testing.
