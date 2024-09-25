#include "postgres.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <pqxx/zview.hxx>
#include <pqxx/result>
#include <iostream>
#include <set>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;
using namespace domain;

void AuthorRepositoryImpl::Save(const Author& author) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(INSERT INTO authors (id, name) VALUES ($1, $2) ON CONFLICT (id) DO UPDATE SET name=$2;)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<Author> AuthorRepositoryImpl::FindAll() const {
    pqxx::read_transaction work(connection_);
    pqxx::result res = work.exec(
        R"(SELECT id, name FROM authors ORDER BY name ASC)"_zv);

    std::vector<Author> authors;
    for (const auto& row : res) {
        std::string id = row["id"].as<std::string>();
        std::string name = row["name"].as<std::string>();
        authors.emplace_back(AuthorId::FromString(id), name);
    }
    return authors;
}

void AuthorRepositoryImpl::Delete(const AuthorId& id) {
    pqxx::work work{connection_};
    work.exec_params(R"(DELETE FROM book_tags WHERE book_id IN (SELECT id FROM books WHERE author_id = $1);)", id.ToString());
    work.exec_params(R"(DELETE FROM books WHERE author_id = $1;)", id.ToString());
    work.exec_params(R"(DELETE FROM authors WHERE id = $1;)", id.ToString());
    work.commit();
}

void AuthorRepositoryImpl::Edit(const AuthorId& id, const std::string& name) {
    pqxx::work work{connection_};
    work.exec_params(R"(UPDATE authors SET name = $1 WHERE id = $2)", name, id.ToString());
    work.commit();
}

void BookRepositoryImpl::Save(const Book& book) {
    pqxx::work work(connection_);
    work.exec_params(
        R"(INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4))"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(), book.GetYear());
    
    if (!book.GetTags().empty()) {
        std::vector<std::string> tags;
        boost::split(tags, book.GetTags(), boost::is_any_of(","));

        for (auto& tag : tags) {
            boost::algorithm::trim(tag);
            work.exec_params(
                R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2))"_zv,
                book.GetId().ToString(), tag);
        }
    }
    work.commit();
}

std::vector<Book> BookRepositoryImpl::FindAll() const {
    pqxx::read_transaction work(connection_);
    pqxx::result res = work.exec(
        R"(SELECT books.id, books.author_id, books.title, books.publication_year, authors.name
        FROM books JOIN authors ON books.author_id = authors.id
        ORDER BY books.title ASC, authors.name ASC, books.publication_year ASC
        )"_zv);

    std::vector<Book> books;
    for (const auto& row : res) {
        std::string id = row["id"].as<std::string>();
        std::string title = row["title"].as<std::string>();
        int year = row["publication_year"].as<int>();
        std::string author_id = row["author_id"].as<std::string>();
        std::string author_name = row["name"].as<std::string>();

        pqxx::result tag_res = work.exec_params(R"(SELECT tag FROM book_tags WHERE book_id = $1 ORDER BY tag ASC)", id);

        std::set<std::string> tags;
        for (const auto& tag_row : tag_res) {
            tags.insert(tag_row["tag"].as<std::string>());
        }

        std::string tag_list = boost::algorithm::join(tags, ", ");

        books.emplace_back(BookId::FromString(id), title, year,
                           AuthorId::FromString(author_id), tag_list, author_name);
    }
    return books;
}

std::vector<Book> BookRepositoryImpl::FindByAuthor(const AuthorId& author_id) const {
    pqxx::read_transaction work(connection_);
    pqxx::result res = work.exec_params(
        R"(SELECT id, title, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year)"_zv,
        author_id.ToString());

    std::vector<Book> books;
    for (const auto& row : res) {
        std::string id = row["id"].as<std::string>();
        std::string title = row["title"].as<std::string>();
        int year = row["publication_year"].as<int>();
        books.emplace_back(BookId::FromString(id), title, year, author_id);
    }
    return books;
}

void BookRepositoryImpl::Delete(const BookId& id) {
    pqxx::work work{connection_};

    work.exec_params(R"(DELETE FROM book_tags WHERE book_id = $1;)", id.ToString());
    work.exec_params(R"(DELETE FROM books WHERE id = $1;)", id.ToString());

    work.commit();
}

void BookRepositoryImpl::Edit(const BookId& book_id, const std::string& title, int publication_year, const std::string& tags) {
    pqxx::work work{connection_};

    work.exec_params(
        R"(UPDATE books SET title = $1, publication_year = $2 WHERE id = $3;)",
        title, publication_year, book_id.ToString()
    );

    work.exec_params(
        R"(DELETE FROM book_tags WHERE book_id = $1;)",
        book_id.ToString()
    );

    if (!tags.empty()) {
        std::vector<std::string> tags_vector;
        boost::split(tags_vector, tags, boost::is_any_of(","));

        for (auto& tag : tags_vector) {
            boost::algorithm::trim(tag);
            if (!tag.empty()) {
                work.exec_params(
                    R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)",
                    book_id.ToString(), tag
                );
            }
        }
    }

    work.commit();
}


Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};

    work.exec(R"(CREATE TABLE IF NOT EXISTS authors (
                    id UUID PRIMARY KEY, 
                    name VARCHAR(100) UNIQUE NOT NULL
                );)"_zv);

    work.exec(R"(CREATE TABLE IF NOT EXISTS books (
                    id UUID PRIMARY KEY, 
                    author_id UUID NOT NULL,
                    title VARCHAR(100) NOT NULL, 
                    publication_year INTEGER NOT NULL,
                    FOREIGN KEY (author_id) REFERENCES authors(id) ON DELETE CASCADE
                );)"_zv);

    work.exec(R"(CREATE TABLE IF NOT EXISTS book_tags (
                    book_id UUID NOT NULL,
                    tag VARCHAR(30) NOT NULL,
                    FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE
                );)"_zv);

    work.commit();
}

}  // namespace postgres