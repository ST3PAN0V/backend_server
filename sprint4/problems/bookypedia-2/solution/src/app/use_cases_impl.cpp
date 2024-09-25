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

void UseCasesImpl::DeleteAuthor(const domain::AuthorId& author_id) {
    authors_.Delete(author_id);
}

void UseCasesImpl::EditAuthor(const domain::AuthorId& author_id, const std::string& name) {
    authors_.Edit(author_id, name);
}


void UseCasesImpl::AddBook(const std::string& title, int year, const AuthorId& author_id, const std::string& tags) {
    books_.Save({BookId::New(), title, year, author_id, tags});
}

std::vector<Book> UseCasesImpl::GetAllBooks() const {
    return books_.FindAll();
}

std::vector<Book> UseCasesImpl::GetBooksByAuthor(const AuthorId& author_id) const {
    return books_.FindByAuthor(author_id);
}

void UseCasesImpl::DeleteBook(const domain::BookId& book_id) {
    books_.Delete(book_id);
}
    
void UseCasesImpl::EditBook(const domain::BookId& book_id, const std::string& title, int publication_year, const std::string& tags) {
    books_.Edit(book_id, title, publication_year, tags);
}


}  // namespace app
