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

    void AddBook(const std::string& title, int year, const domain::AuthorId& author_id) override;
    std::vector<domain::Book> GetAllBooks() const override;
    std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& author_id) const override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
