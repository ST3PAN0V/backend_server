#include <catch2/catch_test_macros.hpp>

#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"
#include "../src/domain/book.h"

#include <optional>

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

    void Delete(const AuthorId& id) {
        std::erase_if(saved_authors, [&id](const auto& author) {
            return author.GetId() == id;
        });
    }
    
    void Edit(const AuthorId& id, const std::string& name) {
        auto it = std::find_if(saved_authors.begin(), saved_authors.end(), [&id](const auto& author) {
            return author.GetId() == id;
        });

        if (it != saved_authors.end()) {
            auto position = saved_authors.erase(it);
            saved_authors.insert(position, domain::Author(id, name));
        }
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

    void Delete(const BookId& id) {
        std::erase_if(saved_books, [&id](const auto& book) {
            return book.GetId() == id;
        });
    }

    void Edit(const domain::BookId& book_id, const std::string& title, int publication_year, const std::string& tags) {
        auto it = std::find_if(saved_books.begin(), saved_books.end(), [&book_id](const auto& book) {
            return book.GetId() == book_id;
        });

        if (it != saved_books.end()) {
            auto author_id = it->GetAuthorId();
            auto author = it->GetAuthor();
            auto position = saved_books.erase(it);
            saved_books.insert(position, domain::Book(book_id, title, publication_year, author_id, tags, author));
        }
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

        auto find_author_by_name = [&](const std::string& name) -> std::optional<domain::AuthorId> {
            auto all_authors = use_cases.GetAllAuthors();
            auto it = std::find_if(all_authors.begin(), all_authors.end(), [&](const auto& author) {
                return author.GetName() == name;
            });
            if (it != all_authors.end()) {
                return std::make_optional(it->GetId());
            }
            return std::nullopt;
        };

        WHEN("Adding an author") {
            const auto author_name = "Author one";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != AuthorId{});
            }
        }

        WHEN("Adding multiple authors") {
            use_cases.AddAuthor("Author one");
            use_cases.AddAuthor("Author two");

            THEN("all authors are saved in the repository") {
                REQUIRE(authors.saved_authors.size() == 2);
                CHECK(authors.saved_authors.at(0).GetName() == "Author one");
                CHECK(authors.saved_authors.at(1).GetName() == "Author two");
            }
        }

        WHEN("An existing author is deleted") {
            const auto author_name = "Author one";
            use_cases.AddAuthor(author_name);
            use_cases.AddAuthor("Author two");

            auto id_opt = find_author_by_name(author_name);
            REQUIRE(id_opt.has_value());
            use_cases.DeleteAuthor(*id_opt);

            THEN("The author is removed from the repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetId() != *id_opt);
            }
        }

        WHEN("A non-existing author is deleted") {
            domain::AuthorId non_existing_id = domain::AuthorId::New();
            use_cases.AddAuthor("Author one");
            use_cases.AddAuthor("Author two");
            use_cases.DeleteAuthor(non_existing_id);

            THEN("The repository remains unchanged") {
                REQUIRE(authors.saved_authors.size() == 2);
            }
        }
 
         WHEN("An existing author's name is changed") {
            const auto author_name = "Author one";
            use_cases.AddAuthor(author_name);

            auto id_opt = find_author_by_name(author_name);
            REQUIRE(id_opt.has_value());

            const std::string new_name = "New Author Name";
            use_cases.EditAuthor(*id_opt, new_name);

            THEN("The author's name is updated in the repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == new_name);
            }
        }

        WHEN("Attempting to edit a non-existing author") {
            use_cases.AddAuthor("Author one");

            domain::AuthorId non_existing_id = domain::AuthorId::New();
            use_cases.EditAuthor(non_existing_id, "New Author name");

            THEN("The repository remains unchanged") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == "Author one");
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

        WHEN("An existing book is deleted") {
            use_cases.AddAuthor("J.K. Rowling");
            const auto author_id = authors.saved_authors.at(0).GetId();
            
            use_cases.AddBook("Harry Potter and the Philosopher's Stone", 1997, author_id);
            use_cases.AddBook("Harry Potter and the Chamber of Secrets", 1998, author_id);
            auto book_id = books.saved_books.at(0).GetId();

            books.Delete(book_id);

            THEN("The book is removed from the repository") {
                REQUIRE(books.saved_books.size() == 1);
                CHECK(books.saved_books.at(0).GetTitle() == "Harry Potter and the Chamber of Secrets");
            }
        }

        WHEN("A non-existing book is deleted") {
            use_cases.AddAuthor("J.K. Rowling");
            const auto author_id = authors.saved_authors.at(0).GetId();
            
            use_cases.AddBook("Harry Potter and the Philosopher's Stone", 1997, author_id);
            use_cases.AddBook("Harry Potter and the Chamber of Secrets", 1998, author_id);

            domain::BookId non_existing_id = domain::BookId::New();
            books.Delete(non_existing_id);

            THEN("The repository remains unchanged") {
                REQUIRE(books.saved_books.size() == 2);
            }
        }
        
        WHEN("An existing book is edited") {
            use_cases.AddAuthor("J.K. Rowling");
            const auto author_id = authors.saved_authors.at(0).GetId();
            
            use_cases.AddBook("Harry Potter and the Philosopher's Stone", 1997, author_id);
            use_cases.AddBook("Harry Potter and the Chamber of Secrets", 1998, author_id);
            auto book_id = books.saved_books.at(0).GetId();

            books.Edit(book_id, "Harry Potter and the Sorcerer's Stone", 1997, "magic, wizard");

            THEN("The book is updated in the repository") {
                REQUIRE(books.saved_books.size() == 2);
                CHECK(books.saved_books.at(0).GetTitle() == "Harry Potter and the Sorcerer's Stone");
                CHECK(books.saved_books.at(0).GetYear() == 1997);
                CHECK(books.saved_books.at(0).GetTags() == "magic, wizard");
            }
        }

        WHEN("Editing a non-existing book") {
            use_cases.AddAuthor("J.K. Rowling");
            const auto author_id = authors.saved_authors.at(0).GetId();
            
            use_cases.AddBook("Harry Potter and the Philosopher's Stone", 1997, author_id);
            use_cases.AddBook("Harry Potter and the Chamber of Secrets", 1998, author_id);

            domain::BookId non_existing_id = domain::BookId::New();
            books.Edit(non_existing_id, "Nonexistent Book", 2000, "nonexistent");

            THEN("The repository remains unchanged") {
                REQUIRE(books.saved_books.size() == 2);
                CHECK(books.saved_books.at(0).GetTitle() == "Harry Potter and the Philosopher's Stone");
                CHECK(books.saved_books.at(1).GetTitle() == "Harry Potter and the Chamber of Secrets");
            }
        }
    }
}