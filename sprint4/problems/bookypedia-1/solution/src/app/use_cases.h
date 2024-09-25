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

    virtual void AddBook(const std::string& title, int year, const domain::AuthorId& author_id) = 0;
    virtual std::vector<domain::Book> GetAllBooks() const = 0;
    virtual std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& author_id) const = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
