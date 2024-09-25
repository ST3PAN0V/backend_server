#include <boost/json.hpp>
#include <pqxx/pqxx>
#include <iostream>
#include <string>

using namespace std::literals;
using pqxx::operator"" _zv;

namespace json = boost::json;

void prepare_statements(pqxx::connection &conn) {
    conn.prepare("add_book"_zv,
        "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULLIF($4, ''))"_zv);
    conn.prepare("all_books"_zv,
        "SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC"_zv);
}

json::value parse_request(const std::string &input) {
    json::error_code ec;
    json::value jv = json::parse(input, ec);
    if (ec) {
        throw std::runtime_error("Invalid JSON input"s);
    }
    return jv;
}

bool add_book(pqxx::connection &conn, const json::object &payload) {
    try {
        pqxx::work txn(conn);
        std::string isbn;
        if (payload.at("ISBN").is_null()) {
            isbn = "";
        } else {
            isbn = json::value_to<std::string>(payload.at("ISBN"));
        }
        
        txn.exec_prepared(
            "add_book"_zv,
            json::value_to<std::string>(payload.at("title"s)),
            json::value_to<std::string>(payload.at("author"s)),
            json::value_to<int>(payload.at("year"s)),
            isbn
        );
        txn.commit();
        return true;
    } catch (const pqxx::sql_error &e) {
        return false;
    }
}

json::array all_books(pqxx::connection &conn) {
    pqxx::read_transaction txn(conn);
    pqxx::result result = txn.exec_prepared("all_books"_zv);

    json::array books;
    for (const auto &row : result) {
        json::object book;
        book["id"s] = row["id"s].as<int>();
        book["title"s] = row["title"s].as<std::string>();
        book["author"s] = row["author"s].as<std::string>();
        book["year"s] = row["year"s].as<int>();
        book["ISBN"s] = row["ISBN"s].is_null() ? json::value(nullptr) : json::value(row["ISBN"s].as<std::string>());

        books.push_back(book);
    }
    return books;
}

void create_books_table(pqxx::connection &conn) {
    pqxx::work txn(conn);
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id SERIAL PRIMARY KEY,
            title VARCHAR(100) NOT NULL,
            author VARCHAR(100) NOT NULL,
            year INTEGER NOT NULL,
            ISBN CHAR(13) UNIQUE
        )
    )"_zv);
    txn.commit();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: "s << argv[0] << " <connection_string>\n"s;
        return 1;
    }

    std::string conn_str = argv[1];
    pqxx::connection conn(conn_str);

    create_books_table(conn);
    prepare_statements(conn);

    std::string input;
    while (std::getline(std::cin, input)) {
        try {
            json::value request = parse_request(input);
            std::string action = json::value_to<std::string>(request.at("action"s));

            if (action == "add_book"s) {
                bool result = add_book(conn, request.at("payload"s).as_object());
                json::object response = { {"result"s, result} };
                std::cout << json::serialize(response) << std::endl;
            } else if (action == "all_books"s) {
                json::array books = all_books(conn);
                std::cout << json::serialize(books) << std::endl;
            } else if (action == "exit"s) {
                break;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error: "s << e.what() << std::endl;
        }
    }

    return 0;
}
