/* libskaidb basic usage in C: connect, create a table, insert with typed
 * parameters, read back, stream a result.
 *
 *   cc -std=c11 -I dist/include basic_usage.c dist/lib/libskaidb.a -lpthread -ldl -lm
 *   ./a.out [host:port] [user] [password]
 */
#include "skaidb.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int fail(const char *what) {
    fprintf(stderr, "%s: %s\n", what, skaidb_last_error());
    return 1;
}

int main(int argc, char **argv) {
    const char *endpoints[] = {argc > 1 ? argv[1] : "127.0.0.1:7000"};
    const char *user = argc > 2 ? argv[2] : "admin";
    const char *password = argc > 3 ? argv[3] : "";

    skaidb_client_t *db = NULL;
    if (skaidb_connect(endpoints, 1, user, password, NULL, &db) != SKAIDB_OK) return fail("connect");

    skaidb_result_t *r = NULL;
    if (skaidb_execute(db, "CREATE TABLE IF NOT EXISTS orders (PRIMARY KEY (id))", &r) != SKAIDB_OK)
        return fail("create table");
    skaidb_result_free(r);

    /* Typed parameters: the values are yours to free. */
    skaidb_prepared_t *ins = NULL;
    if (skaidb_prepare(db, "INSERT INTO orders (id, customer, total, paid) VALUES (?, ?, ?, ?)", &ins) != SKAIDB_OK)
        return fail("prepare");
    for (int i = 1; i <= 3; i++) {
        skaidb_value_t *params[4] = {
            skaidb_value_int(i),
            skaidb_value_string(i % 2 ? "ada" : "grace"),
            skaidb_value_float(10.5 * i),
            skaidb_value_bool(i != 2),
        };
        if (skaidb_execute_prepared(db, ins, (const skaidb_value_t *const *)params, 4, &r) != SKAIDB_OK)
            return fail("insert");
        skaidb_result_free(r);
        for (int j = 0; j < 4; j++) skaidb_value_free(params[j]);
    }
    skaidb_prepared_free(ins);

    /* A whole result set in memory. */
    if (skaidb_execute(db, "SELECT id, customer, total FROM orders WHERE paid = true ORDER BY id", &r) != SKAIDB_OK)
        return fail("select");
    for (size_t i = 0; i < skaidb_result_row_count(r); i++) {
        size_t len;
        const char *customer = skaidb_value_get_string(skaidb_result_cell(r, i, 1), &len);
        printf("order %" PRId64 " by %.*s: %.2f\n", skaidb_value_get_int(skaidb_result_cell(r, i, 0)),
               (int)len, customer, skaidb_value_get_float(skaidb_result_cell(r, i, 2)));
    }
    skaidb_result_free(r);

    /* Streaming keeps one chunk resident: the way to read a big table. */
    skaidb_stream_t *stream = NULL;
    if (skaidb_query_stream(db, "SELECT id, total FROM orders", &stream) != SKAIDB_OK) return fail("stream");
    const skaidb_value_t *const *row;
    size_t n;
    double sum = 0;
    while (skaidb_stream_next(stream, &row, &n) == SKAIDB_OK) sum += skaidb_value_get_float(row[1]);
    skaidb_stream_free(stream);
    printf("total of all orders: %.2f\n", sum);

    skaidb_execute(db, "DROP TABLE orders", &r);
    skaidb_result_free(r);
    skaidb_client_free(db);
    return 0;
}
