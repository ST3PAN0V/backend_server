#pragma once
#include "../domain/author_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}, books_{books} {}

    void AddAuthor(const std::string& name) override;
    std::vector<domain::Author> GetAllAuthors() const override;
    void DeleteAuthor(const domain::AuthorId& author_id) override;
    void EditAuthor(const domain::AuthorId& author_id, const std::string& name) override;

    void AddBook(const std::string& title, int year, const domain::AuthorId& author_id, const std::string& tags = {}) override;
    std::vector<domain::Book> GetAllBooks() const override;
    std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& author_id) const override;
    void DeleteBook(const domain::BookId& book_id) override;
    void EditBook(const domain::BookId& book_id, const std::string& title, int publication_year, const std::string& tags) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
