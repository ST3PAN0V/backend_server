#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/result>
#include <iostream>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(INSERT INTO authors (id, name) VALUES ($1, $2) ON CONFLICT (id) DO UPDATE SET name=$2;)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::FindAll() const {
    pqxx::read_transaction work(connection_);
    pqxx::result res = work.exec(
        R"(SELECT id, name FROM authors ORDER BY name ASC)"_zv);

    std::vector<domain::Author> authors;
    for (const auto& row : res) {
        std::string id = row["id"].as<std::string>();
        std::string name = row["name"].as<std::string>();
        authors.emplace_back(domain::AuthorId::FromString(id), name);
    }
    return authors;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work(connection_);
    work.exec_params(
        R"(INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4))"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetYear());
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::FindAll() const {
    pqxx::read_transaction work(connection_);
    pqxx::result res = work.exec(
        R"(SELECT id, author_id, title, publication_year FROM books ORDER BY title ASC)"_zv);

    std::vector<domain::Book> books;
    for (const auto& row : res) {
        std::string id = row["id"].as<std::string>();
        std::string title = row["title"].as<std::string>();
        int year = row["publication_year"].as<int>();
        std::string author_id = row["author_id"].as<std::string>();
        books.emplace_back(domain::BookId::FromString(id), title, year, domain::AuthorId::FromString(author_id));
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::FindByAuthor(const domain::AuthorId& author_id) const {
    pqxx::read_transaction work(connection_);
    pqxx::result res = work.exec_params(
        R"(SELECT id, title, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year)"_zv,
        author_id.ToString());

    std::vector<domain::Book> books;
    for (const auto& row : res) {
        std::string id = row["id"].as<std::string>();
        std::string title = row["title"].as<std::string>();
        int year = row["publication_year"].as<int>();
        books.emplace_back(domain::BookId::FromString(id), title, year, author_id);
    }
    return books;
}


Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(
        R"(
            CREATE TABLE IF NOT EXISTS authors (
            id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
            name varchar(100) UNIQUE NOT NULL
        );)"_zv);


    work.exec(
        R"(CREATE TABLE IF NOT EXISTS books (
            id UUID CONSTRAINT book_id_constraint PRIMARY KEY, 
            author_id UUID NOT NULL,
            title VARCHAR(100) NOT NULL, 
            publication_year INTEGER NOT NULL,
            CONSTRAINT fk_author FOREIGN KEY(author_id) REFERENCES authors(id) ON DELETE CASCADE
        );)"_zv
    );

    work.commit();
}

}  // namespace postgres