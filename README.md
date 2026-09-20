# skaidb C and C++ driver

`libskaidb` is the official C driver for [skaidb](https://skaidb.org), with a
header-only C++17 layer on top. It is a thin C ABI over the reference Rust
driver, so it speaks the same binary protocol and carries every feature of
it: SCRAM-SHA-256 and Kerberos logins, TLS, nearest-node selection and
failover across a seed list, prepared statements with typed parameters,
pipelining, streamed result sets and a connection pool.

- `include/skaidb.h` — the C API: opaque handles, a status code from every
  fallible call with the message in the thread-local `skaidb_last_error()`,
  one `skaidb_*_free()` per handle.
- `include/skaidb.hpp` — C++17 RAII over it: `skaidb::Client`, `Value`,
  `Result`, `Stream`, `Pool`; `skaidb::Error` carries the status; range-for
  over results and streams.

Versions follow the skaidb release the library was built and tested with:
`libskaidb 0.294.2` is the driver of server 0.294.2, and the protocol is
backward compatible in both directions.

## Install

**Prebuilt** (no Rust toolchain needed): every
[release](https://github.com/porcupin26/skaidb-c/releases) attaches
`libskaidb-<version>-<target>.tar.gz` for Linux x86_64 and aarch64 and for
macOS, each holding `include/` and `lib/` (`libskaidb.a` plus the shared
library) and a CMake package config. Unpack it and either add the paths by
hand or point CMake at it:

```sh
tar xzf libskaidb-0.294.2-x86_64-unknown-linux-gnu.tar.gz
cc  -std=c11   -I libskaidb/include app.c   libskaidb/lib/libskaidb.a -lpthread -ldl -lm
c++ -std=c++17 -I libskaidb/include app.cpp libskaidb/lib/libskaidb.a -lpthread -ldl -lm
```

```cmake
list(APPEND CMAKE_PREFIX_PATH "/path/to/libskaidb")
find_package(skaidb REQUIRED)
target_link_libraries(app PRIVATE skaidb::skaidb)
```

**From source** (needs [cargo](https://rustup.rs)): this repository pins the
skaidb version in `SKAIDB_VERSION` and builds the library from the
[skaidb-rust](https://github.com/porcupin26/skaidb-rust) repository at that
tag.

```sh
scripts/build.sh              # -> dist/include, dist/lib
# or, through CMake, which also builds the examples and tests:
cmake -S . -B build && cmake --build build
```

Kerberos logins need the library built with the `kerberos` feature
(`SKAIDB_FEATURES=kerberos scripts/build.sh`, MIT krb5 development headers
installed); without it `skaidb_connect_gssapi` returns `SKAIDB_ERR_UNSUPPORTED`.

## Quick start

C:

```c
#include "skaidb.h"

const char *endpoints[] = {"db1:7000", "db2:7000"};
skaidb_client_t *db;
if (skaidb_connect(endpoints, 2, "app", "secret", NULL, &db) != SKAIDB_OK) {
    fprintf(stderr, "%s\n", skaidb_last_error());
    return 1;
}
skaidb_prepared_t *ins;
skaidb_prepare(db, "INSERT INTO orders (id, total) VALUES (?, ?)", &ins);
skaidb_value_t *params[] = {skaidb_value_int(42), skaidb_value_float(12.5)};
skaidb_result_t *r;
skaidb_execute_prepared(db, ins, (const skaidb_value_t *const *)params, 2, &r);
skaidb_result_free(r);
skaidb_value_free(params[0]); skaidb_value_free(params[1]);
skaidb_prepared_free(ins);

skaidb_execute(db, "SELECT id, total FROM orders ORDER BY id", &r);
for (size_t i = 0; i < skaidb_result_row_count(r); i++)
    printf("%lld %f\n", (long long)skaidb_value_get_int(skaidb_result_cell(r, i, 0)),
           skaidb_value_get_float(skaidb_result_cell(r, i, 1)));
skaidb_result_free(r);
skaidb_client_free(db);
```

C++:

```cpp
#include "skaidb.hpp"

auto db = skaidb::Client::connect({"db1:7000", "db2:7000"}, "app", "secret");
auto ins = db.prepare("INSERT INTO orders (id, total) VALUES (?, ?)");
db.execute(ins, {42, 12.5});
for (skaidb::Row row : db.execute("SELECT id, total FROM orders ORDER BY id"))
    std::cout << row[0].as_int() << ' ' << row[1].as_float() << '\n';
for (const skaidb::Row &row : db.query_stream("SELECT * FROM big_table"))
    use(row);   // one chunk resident at a time
```

`examples/basic_usage.c` and `examples/basic_usage.cpp` are complete
programs; `tests/smoke.c` and `tests/smoke.cpp` exercise every call of both
APIs against a live server and double as reference usage.

## The API in one page

| Area | C | C++ |
|------|---|-----|
| Connect | `skaidb_connect(endpoints, n, user, password, tls, &client)`, `skaidb_connect_anonymous`, `skaidb_connect_gssapi` | `skaidb::Client::connect(endpoints, user, password, tls)`, `connect_anonymous`, `connect_gssapi` |
| TLS | `skaidb_tls_t { verify, ca_file, server_name }`; `NULL` for plaintext | `skaidb::Tls::ca(path)`, `Tls::system()`, `Tls::insecure()` |
| Session | `skaidb_client_use_database`, `_set_consistency` (`SKAIDB_ONE/QUORUM/ALL`), `_set_scan_budget_rows`, `_add_endpoints`, `_reconnect` | `use_database`, `set_consistency(Consistency::Quorum)`, `set_scan_budget_rows`, `add_endpoints`, `reconnect` |
| Statements | `skaidb_execute[_with]`, `skaidb_prepare` + `skaidb_execute_prepared[_with]`, `skaidb_execute_batch`, `skaidb_pipeline` | `execute(sql[, consistency])`, `prepare` + `execute(stmt, {params})`, `execute_batch(stmt, rows)`, `pipeline({sql...})` |
| Results | `skaidb_result_kind/column_count/column_name/row_count/cell/affected/error/set_count/set` | `Result`: `kind()`, `columns()`, `row_count()`, `row(i)` / `[i]`, `cell(r, c)`, `affected()`, `error()`, range-for |
| Streaming | `skaidb_query_stream[_with]`, `skaidb_stream_next(stream, &row, &n)` → `SKAIDB_OK` / `SKAIDB_END`, `skaidb_stream_free` (drains) | `query_stream(sql)`, `next()` → `optional<Row>`, range-for |
| Change streams | `skaidb_stream_poll(client, stream, after, limit, &events, &cursor)` | `stream_poll(name, after, limit)` → `{events, next_cursor}` |
| Values | `skaidb_value_null/bool/int/float/decimal/string/bytes/uuid/timestamp/array/document/from_json`; `skaidb_value_type/get_*/array_get/document_get`; `skaidb_value_to_json_dup` | `skaidb::Value` (implicit from `bool`, integers, `double`, strings), `Value::decimal/bytes/uuid/timestamp/array/document/from_json`; `ValueView::as_int()`, `as_string()`, `get("a.b")`, `json()` |
| Pool | `skaidb_pool_new`, `_acquire`, `_release`, `_idle_len`, `_free` | `skaidb::Pool`, `acquire()` → `Lease` (returns the connection when destroyed), `with(fn)` |

Rules that keep it safe: every object is created and freed by the library;
pointers obtained from a result, stream or value borrow it and are valid
until it is freed (a stream's row until the next `skaidb_stream_next`);
parameter values stay the caller's; a client is one connection used from one
thread at a time; a pool is thread-safe; an open stream borrows its client
until it is freed. Placeholders are `?`.

## Tests

`tests/` holds the C and C++ smoke tests. They need a server:

```sh
SKAIDB_ENDPOINT=127.0.0.1:7000 SKAIDB_USER=admin SKAIDB_PASSWORD=secret ctest --test-dir build
```

CI builds the library from the pinned skaidb-rust tag, downloads the matching
server release and runs both suites against it on every push.

## Source of truth and versions

The library's source is `crates/skaidb-ffi` in the
[skaidb](https://github.com/porcupin26/skaidb) repository, carried by the
[skaidb-rust](https://github.com/porcupin26/skaidb-rust) repository at the
same tags; this repository holds the headers, examples, tests, CMake
integration and the release packaging. Tags here (`vX.Y.Z`) are skaidb
release versions. Issues are welcome on this tracker.

License: SSPL-1.0, like skaidb.
