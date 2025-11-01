#ifndef ITERABLE_PER_STRING_HPP
#define ITERABLE_PER_STRING_HPP

#include <iostream>
#include <iterator>
#include <locale>
#include <string>
#include <vector>

class per_line_stream
{
public:
    explicit per_line_stream(std::istream& source):m_source(source)
    {
        set_per_line();
    }
    per_line_stream() = delete;
    per_line_stream(const per_line_stream &) = delete;
    per_line_stream(per_line_stream &&) = delete;
    per_line_stream&operator=(const per_line_stream &) = delete;
    per_line_stream&operator=(per_line_stream &&) = delete;
    ~per_line_stream()
    {
        restore_per_line();
    }

    auto begin()
    {
        return std::istream_iterator<std::string>(m_source);
    }

    auto end()
    {
        return std::istream_iterator<std::string>();
    }

    void reset()
    {
        m_source.clear();
        m_source.seekg(0, std::ios_base::beg);
    }
private:
    struct line_reader: std::ctype<char>
    {
        line_reader(): std::ctype<char>(get_table()) {}
        static std::ctype_base::mask const* get_table()
        {
            static std::vector<std::ctype_base::mask>
              rc(table_size, std::ctype_base::mask());

            rc['\n'] = std::ctype_base::space;
            return rc.data();
        }
    };

    void set_per_line()
    {
        m_original = m_source.imbue(std::locale(m_def_locale, new line_reader()));
    }

    void restore_per_line()
    {
        m_source.imbue(m_original);
    }

    std::istream& m_source;
    std::locale m_original;
    line_reader m_reader;
    const std::locale m_def_locale;
};
#endif
