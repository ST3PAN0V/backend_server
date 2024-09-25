#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/join.hpp>

#include <cassert>
#include <iostream>
#include <ranges>
#include <sstream>
#include <set>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << " by " << book.author << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1));
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s, std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowBook"s, "title"s, "Show book"s, std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s, std::bind(&View::ShowAuthorBooks, this));
    menu_.AddAction("DeleteAuthor"s, "name"s, "Delete author"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "name"s, "Edit author"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "title"s, "Delete book"s, std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditBook"s, "title"s, "Edit book"s, std::bind(&View::EditBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        if (name.empty()) {
            throw std::runtime_error("Invalid author name");
        }
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);
    domain::AuthorId author_id;

    if (name.empty()) {
        auto author_id_opt = SelectAuthor();
        if (author_id_opt) {
            author_id = domain::AuthorId::FromString(*author_id_opt);
        }
    }
    else {
        auto author_opt = FindAuthorByName(name);
        if (author_opt) {
            author_id = domain::AuthorId::FromString(author_opt->id);
        }
    }

    if (!author_id) {
        std::cout << "Failed to delete author.\n";
        return true;
    }

    use_cases_.DeleteAuthor(author_id);
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);
    domain::AuthorId author_id;

    if (name.empty()) {
        auto author_id_opt = SelectAuthor();
        if (author_id_opt) {
            author_id = domain::AuthorId::FromString(*author_id_opt);
        }
    }
    else {
        auto author_opt = FindAuthorByName(name);
        if (author_opt) {
            author_id = domain::AuthorId::FromString(author_opt->id);
        }
    }

    if (!author_id) {
        output_ << "Failed to edit author.\n";
        return true;
    }

    output_ << "Enter new name:\n";
    std::string new_name;
    std::getline(input_, new_name);
    boost::algorithm::trim(new_name);

    use_cases_.EditAuthor(author_id, new_name);
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    int publication_year;
    cmd_input >> publication_year;

    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);

    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string author_name;
    std::getline(input_, author_name);
    boost::algorithm::trim(author_name);

    domain::AuthorId author_id;

    if (author_name.empty()) {
        auto author_id_opt = SelectAuthor();
        if (author_id_opt) {
            author_id = domain::AuthorId::FromString(*author_id_opt);
        }
        else {
            // std::string tags;
            // std::getline(input_, tags);
            return true;
        }
    }
    else {
        auto author_opt = FindAuthorByName(author_name);
        if (author_opt) {
            author_id = domain::AuthorId::FromString(author_opt->id);
        }
        else {
            output_ << "No author found. Do you want to add " << author_name << " (y/n)? " << std::endl;
            std::string answer;
            std::getline(input_, answer);
            if (answer == "y" || answer == "Y") {
                use_cases_.AddAuthor(author_name);
                author_opt = FindAuthorByName(author_name);
                if (author_opt) {
                    author_id = domain::AuthorId::FromString(author_opt->id);
                }
            }
        }
    }

    if (!author_id) {
        output_ << "Failed to add book." << std::endl;
        return true;
    }

    output_ << "Enter tags (comma separated):" << std::endl;
    std::string tags_input;
    std::getline(input_, tags_input);

    auto tags = NormalizeTags(tags_input);

    use_cases_.AddBook(title, publication_year, author_id, tags);

    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input,title);
    boost::algorithm::trim(title);

    std::vector<detail::BookInfo> books;
    if (title.empty()) {
        books = GetBooks();
    }
    else {
        std::ranges::copy_if(GetBooks(), std::back_inserter(books), [&title](const auto& book) {
            return book.title == title;
        });
    }

    if (books.empty()) {
        return true;
    }

    int index = 0;
    if (books.size() > 1) {
        auto index_opt = SelectBook(books);
        if (!index_opt) {
            return true;
        }
        index = *index_opt;
    }

    const auto& book = books[index];
    output_ << "Title: " << book.title << "\n";
    output_ << "Author: " << book.author << "\n";
    output_ << "Publication year: " << book.publication_year << "\n";
    if (!book.tags.empty()) {
        output_ << "Tags: " << book.tags << "\n";
    }

    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::invalid_argument& e) {
        output_ << "Invalid input: " << e.what() << "\n";
        return false;
    } catch (const std::runtime_error& e) {
        output_ << "Runtime error: " << e.what() << "\n";
        return false;
    } catch (const std::exception& e) {
        output_ << "An unexpected error occurred: " << e.what() << "\n";
        return false;
    }

    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input,title);
    boost::algorithm::trim(title);

    std::vector<detail::BookInfo> books;
    if (title.empty()) {
        books = GetBooks();
    }
    else {
        std::ranges::copy_if(GetBooks(), std::back_inserter(books), [&title](const auto& book) {
            return book.title == title;
        });
    }

    if (books.empty()) {
        return true;
    }

    int index = 0;
    if (books.size() > 1) {
        auto index_opt = SelectBook(books);
        if (!index_opt) {
            return true;
        }
        index = *index_opt;
    }

    const auto& book = books[index];
    use_cases_.DeleteBook(domain::BookId::FromString(book.id));
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input,title);
    boost::algorithm::trim(title);

    std::vector<detail::BookInfo> books;
    if (title.empty()) {
        books = GetBooks();
    }
    else {
        std::ranges::copy_if(GetBooks(), std::back_inserter(books), [&title](const auto& book) {
            return book.title == title;
        });
    }

    if (books.empty()) {
        output_ << "Book not found\n";
        return true;
    }

    int index = 0;
    if (books.size() > 1) {
        auto index_opt = SelectBook(books);
        if (!index_opt) {
            output_ << "Book not found\n";
            return true;
        }
        index = *index_opt;
    }

    auto book = books[index];
 
    // новое название
    output_ << "Enter new title or empty line to use the current one (" << book.title << "):" << std::endl;
    std::string new_title;
    std::getline(input_, new_title);
    boost::algorithm::trim(new_title);

    if (!new_title.empty()) {
        book.title = new_title;
    }

    // новый год публикации
    output_ << "Enter publication year or empty line to use the current one (" << book.publication_year << "):" << std::endl;
    std::string new_year_str;
    std::getline(input_, new_year_str);
    if (!new_year_str.empty()) {
        try {
            auto new_year = std::stoi(new_year_str);
            book.publication_year = new_year;
        } catch (...) {
        }
    }

    // новые теги
    output_ << "Enter tags (current tags: " << book.tags << "):" << std::endl;
    std::string new_tags_input;
    std::getline(input_, new_tags_input);
    auto new_tags = NormalizeTags(new_tags_input);
    book.tags = new_tags;

    use_cases_.EditBook(domain::BookId::FromString(book.id), book.title, book.publication_year, book.tags);
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    auto author_id = SelectAuthor();
    if (not author_id.has_value())
        return std::nullopt;
    else {
        params.author_id = author_id.value();
        return params;
    }
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        return std::nullopt;
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        return std::nullopt;
    }

    return authors[author_idx].id;
}

std::optional<int> View::SelectBook(const std::vector<detail::BookInfo>& books) const {
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        return std::nullopt;
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        return std::nullopt;
    }

    return book_idx;
}


std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_autors;

    std::ranges::transform(use_cases_.GetAllAuthors(), std::back_inserter(dst_autors), [](const auto& author) {
        return detail::AuthorInfo{author.GetId().ToString(), author.GetName()};
    });

    return dst_autors;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    std::vector<detail::BookInfo> books;
    std::ranges::transform(use_cases_.GetAllBooks(), std::back_inserter(books), [](const auto& book) {
        return detail::BookInfo{book.GetId().ToString(), book.GetTitle(), book.GetAuthor(), book.GetYear(), book.GetTags()};
    });
    
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;

    std::ranges::transform(use_cases_.GetBooksByAuthor(domain::AuthorId::FromString(author_id)), std::back_inserter(books), [](const auto& book) {
        return detail::BookInfo{book.GetId().ToString(), book.GetTitle(), book.GetAuthor(), book.GetYear(), book.GetTags()};
    });

    return books;
}

std::optional<detail::AuthorInfo> View::FindAuthorByName(const std::string& name) const {
    auto authors = use_cases_.GetAllAuthors();
    auto it = std::ranges::find_if(authors, [&name](const auto& author) {
        return author.GetName() == name;
    });

    if (it != authors.end()) {
        return detail::AuthorInfo{it->GetId().ToString(), name};
    }
    
    return std::nullopt;
}

std::string View::NormalizeTags(const std::string& tags_input) const {
    std::istringstream iss(tags_input);
    std::string tag;
    std::set<std::string> unique_tags;

    while (std::getline(iss, tag, ',')) {
        boost::algorithm::trim(tag);
        if (!tag.empty()) {
            unique_tags.insert(tag);
        }
    }

    return boost::algorithm::join(unique_tags, ", ");
}


}  // namespace ui
