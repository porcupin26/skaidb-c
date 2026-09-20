# Smoke tests

`smoke.c` and `smoke.cpp` exercise every call of the C and C++ APIs against
a live server: DDL, typed parameters, batches, every value accessor, a
pipeline with an inline error, streaming (including an abandoned stream),
values on their own, and the pool. They are the same programs the skaidb
repository runs in its own CI.

```sh
cmake -S . -B build && cmake --build build
SKAIDB_ENDPOINT=127.0.0.1:7000 SKAIDB_USER=admin SKAIDB_PASSWORD=secret \
  ctest --test-dir build --output-on-failure
```

They create and drop tables named `cffi_smoke` and `cppffi_smoke` in the
session database. A plaintext endpoint is assumed; for a TLS-only server
edit the connect call to pass a `skaidb_tls_t` / `skaidb::Tls`.
