#include <catch2/catch_test_macros.hpp>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"
#include "../src/domain/book.h"

using namespace domain;

namespace {

struct MockAuthorRepository : AuthorRepository {
    std::vector<Author> saved_authors;

    void Save(const Author& author) override {
        saved_authors.emplace_back(author);
    }

    std::vector<Author> FindAll() const override {
        return saved_authors;
    }
};

struct MockBookRepository : BookRepository {
    std::vector<Book> saved_books;

    void Save(const Book& book) override {
        saved_books.emplace_back(book);
    }

    std::vector<Book> FindAll() const override {
        return saved_books;
    }

    std::vector<Book> FindByAuthor(const AuthorId& author_id) const override {
        std::vector<Book> books_by_author;

    std::copy_if(saved_books.begin(), saved_books.end(), std::back_inserter(books_by_author),[&author_id](const Book& book) {
        return book.GetAuthorId() == author_id;
    });
    
    return books_by_author;
    }
};

struct Fixture {
    MockAuthorRepository authors;
    MockBookRepository books;
};

}  // namespace

SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{authors, books};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != AuthorId{});
            }
        }

        WHEN("Adding multiple authors") {
            use_cases.AddAuthor("J.K. Rowling");
            use_cases.AddAuthor("George R.R. Martin");

            THEN("all authors are saved in the repository") {
                REQUIRE(authors.saved_authors.size() == 2);
                CHECK(authors.saved_authors.at(0).GetName() == "J.K. Rowling");
                CHECK(authors.saved_authors.at(1).GetName() == "George R.R. Martin");
            }
        }

        WHEN("Adding a book after adding an author") {
            const auto author_name = "J.K. Rowling";
            use_cases.AddAuthor(author_name);

            const auto title = "Harry Potter and the Philosopher's Stone";
            const int publication_year = 1997;
            const auto author_id = authors.saved_authors.at(0).GetId();

            use_cases.AddBook(title, publication_year, author_id);

            THEN("book is saved to repository") {
                REQUIRE(books.saved_books.size() == 1);
                CHECK(books.saved_books.at(0).GetTitle() == title);
                CHECK(books.saved_books.at(0).GetYear() == publication_year);
                CHECK(books.saved_books.at(0).GetAuthorId() == author_id);
            }
        }
        WHEN("Adding multiple books for an author") {
            use_cases.AddAuthor("J.K. Rowling");
            const auto author_id = authors.saved_authors.at(0).GetId();

            use_cases.AddBook("Harry Potter and the Philosopher's Stone", 1997, author_id);
            use_cases.AddBook("Harry Potter and the Chamber of Secrets", 1998, author_id);

            THEN("all books are saved in the repository") {
                REQUIRE(books.saved_books.size() == 2);
                CHECK(books.saved_books.at(0).GetTitle() == "Harry Potter and the Philosopher's Stone");
                CHECK(books.saved_books.at(1).GetTitle() == "Harry Potter and the Chamber of Secrets");
            }
        }

        WHEN("Querying all authors") {
            use_cases.AddAuthor("J.K. Rowling");
            use_cases.AddAuthor("George R.R. Martin");

            const auto all_authors = use_cases.GetAllAuthors();

            THEN("all authors are returned") {
                REQUIRE(all_authors.size() == 2);
                CHECK(all_authors.at(0).GetName() == "J.K. Rowling");
                CHECK(all_authors.at(1).GetName() == "George R.R. Martin");
            }
        }

        WHEN("Querying books by an author") {
            use_cases.AddAuthor("J.K. Rowling");
            const auto author_id = authors.saved_authors.at(0).GetId();

            use_cases.AddBook("Harry Potter and the Philosopher's Stone", 1997, author_id);
            use_cases.AddBook("Harry Potter and the Chamber of Secrets", 1998, author_id);

            const auto books_by_author = use_cases.GetBooksByAuthor(author_id);

            THEN("only books by that author are returned") {
                REQUIRE(books_by_author.size() == 2);
                CHECK(books_by_author.at(0).GetTitle() == "Harry Potter and the Philosopher's Stone");
                CHECK(books_by_author.at(1).GetTitle() == "Harry Potter and the Chamber of Secrets");
            }
        }
    }
}