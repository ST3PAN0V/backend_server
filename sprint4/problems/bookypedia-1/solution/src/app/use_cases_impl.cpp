#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

std::vector<Author> UseCasesImpl::GetAllAuthors() const {
    return authors_.FindAll();
}

void UseCasesImpl::AddBook(const std::string& title, int year, const AuthorId& author_id) {
    books_.Save({BookId::New(), title, year, author_id});
}

std::vector<Book> UseCasesImpl::GetAllBooks() const {
    return books_.FindAll();
}

std::vector<Book> UseCasesImpl::GetBooksByAuthor(const AuthorId& author_id) const {
    return books_.FindByAuthor(author_id);
}


}  // namespace app
