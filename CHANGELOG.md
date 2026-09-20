# Changelog

Versions are skaidb release versions: `libskaidb X.Y.Z` is the driver built
and tested with server X.Y.Z.

## [0.294.2]

- First release of the C driver and the header-only C++17 layer: connect
  (SCRAM, anonymous, Kerberos), TLS, session database and consistency,
  prepared statements with typed parameters, batches, pipelines, streamed
  result sets, change-stream polling, the value model (null, bool, int,
  float, decimal, string, bytes, uuid, timestamp, array, document, JSON in
  and out) and the connection pool.
- Prebuilt archives for Linux x86_64, Linux aarch64 and macOS, a CMake
  package config, examples and smoke tests.
