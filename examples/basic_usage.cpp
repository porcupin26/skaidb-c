// libskaidb basic usage in C++17: the RAII layer over the C API.
//
//   c++ -std=c++17 -I dist/include basic_usage.cpp dist/lib/libskaidb.a -lpthread -ldl -lm
//   ./a.out [host:port] [user] [password]
#include "skaidb.hpp"

#include <iostream>

int main(int argc, char **argv) {
    const std::string endpoint = argc > 1 ? argv[1] : "127.0.0.1:7000";
    const std::string user = argc > 2 ? argv[2] : "admin";
    const std::string password = argc > 3 ? argv[3] : "";
    try {
        auto db = skaidb::Client::connect(endpoint, user, password);
        db.execute("CREATE TABLE IF NOT EXISTS orders (PRIMARY KEY (id))");

        auto ins = db.prepare("INSERT INTO orders (id, customer, total, paid) VALUES (?, ?, ?, ?)");
        db.execute(ins, {1, "ada", 10.5, true});
        db.execute_batch(ins, {{2, "grace", 21.0, false}, {3, "ada", 31.5, true}});

        for (skaidb::Row row : db.execute("SELECT id, customer, total FROM orders WHERE paid = true ORDER BY id"))
            std::cout << "order " << row[0].as_int() << " by " << row[1].as_string() << ": " << row[2].as_float()
                      << '\n';

        double sum = 0;
        for (const skaidb::Row &row : db.query_stream("SELECT id, total FROM orders")) sum += row[1].as_float();
        std::cout << "total of all orders: " << sum << '\n';

        db.execute("DROP TABLE orders");
    } catch (const skaidb::Error &e) {
        std::cerr << "skaidb error (status " << e.status() << "): " << e.what() << '\n';
        return 1;
    }
    return 0;
}
