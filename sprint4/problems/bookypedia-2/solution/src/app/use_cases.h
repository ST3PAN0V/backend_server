#pragma once

#include <string>
#include <vector>

#include "../domain/book.h"
#include "../domain/author.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual std::vector<domain::Author> GetAllAuthors() const = 0;
    virtual void DeleteAuthor(const domain::AuthorId& author_id) = 0;
    virtual void EditAuthor(const domain::AuthorId& author_id, const std::string& name) = 0;

    virtual void AddBook(const std::string& title, int year, const domain::AuthorId& author_id, const std::string& tags) = 0;
    virtual std::vector<domain::Book> GetAllBooks() const = 0;
    virtual std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& author_id) const = 0;
    virtual void DeleteBook(const domain::BookId& book_id) = 0;
    virtual void EditBook(const domain::BookId& book_id, const std::string& title, int publication_year, const std::string& tags) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
