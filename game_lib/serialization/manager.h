#pragma once

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <fstream>
#include <filesystem>
#include <vector>
#include <iostream>

namespace serialization {

class Manager {
public:
    enum class Mode { Read, Write };

    explicit Manager(const std::string& filename, Mode mode)
    : filename_(filename)
    , mode_(mode) {
        if (mode_ == Mode::Read) {
            if (std::filesystem::exists(filename_)) {
                ifs_.open(filename_, std::ios::binary);
                if (!ifs_.is_open()) {
                    throw std::runtime_error("Failed to open file for reading: " + filename_);
                }
            }
        }
        else if (mode_ == Mode::Write) {
            if (!filename_.empty()) {
                tmp_filename_ = filename_ + ".tmp";  // Временный файл для записи
                ofs_.open(tmp_filename_, std::ios::binary);
                if (!ofs_.is_open()) {
                    throw std::runtime_error("Failed to open temporary file for writing: " + tmp_filename_);
                }
            }
        }
    }

    ~Manager() {
        if (ifs_.is_open()) {
            ifs_.close();
        }
        if (ofs_.is_open()) {
            ofs_.close();
        }
        if (mode_ == Mode::Write && !filename_.empty()) {
            try{
                std::filesystem::rename(tmp_filename_, filename_);
            }
            catch(const std::exception& e) {
                std::cout << e.what() << std::endl;
            }
        }
    }

    template <typename T>
    void Read(std::vector<T>& objects) {
        if (mode_ != Mode::Read || !ifs_.is_open()) {
            return;
        }

        try {
            boost::archive::text_iarchive ia(ifs_);
            ia >> objects;
        }
        catch (const boost::archive::archive_exception& e) {
            throw std::runtime_error("Boost archive exception: " + std::string(e.what()));
        }
        catch (const std::exception& e) {
            throw std::runtime_error("General exception: " + std::string(e.what()));
        }
    }

    template <typename T>
    void Write(const std::vector<T>& objects) {
        if (mode_ != Mode::Write || !ofs_.is_open()) {
            return;
        }

        try {
            boost::archive::text_oarchive oa(ofs_);
            oa << objects;
        }
        catch (const boost::archive::archive_exception& e) {
            throw std::runtime_error("Boost archive exception: " + std::string(e.what()));
        }
        catch (const std::exception& e) {
            throw std::runtime_error("General exception: " + std::string(e.what()));
        }
    }

private:
    std::string filename_;
    std::string tmp_filename_;
    Mode mode_;
    std::ifstream ifs_;
    std::ofstream ofs_;
};

} // namespace serialization