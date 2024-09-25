#pragma once

#include <string>
#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId id, std::string title, int year, AuthorId author_id)
        : id_(std::move(id)), title_(std::move(title)), year_(year), author_id_(std::move(author_id)) {}

    const BookId& GetId() const noexcept {
        return id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    int GetYear() const noexcept {
        return year_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

private:
    BookId id_;
    std::string title_;
    int year_;
    AuthorId author_id_;
};

class BookRepository {
public:
    virtual void Save(const Book& book) = 0;
    virtual std::vector<Book> FindAll() const = 0;
    virtual std::vector<Book> FindByAuthor(const AuthorId& author_id) const = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain
