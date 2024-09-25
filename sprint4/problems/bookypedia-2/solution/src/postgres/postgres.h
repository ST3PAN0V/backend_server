#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> FindAll() const override;
    void Delete(const domain::AuthorId& id) override;
    void Edit(const domain::AuthorId& id, const std::string& name) override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_(connection) {}

    void Save(const domain::Book& book) override;
    std::vector<domain::Book> FindAll() const override;
    std::vector<domain::Book> FindByAuthor(const domain::AuthorId& author_id) const override;
    void Delete(const domain::BookId& id) override;
    void Edit(const domain::BookId& book_id, const std::string& title, int publication_year, const std::string& tags) override;

private:
    pqxx::connection& connection_;
};


class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{connection_};
};

}  // namespace postgres