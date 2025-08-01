#include <lexer.hpp>

int gettok()
{
    static char last_char = ' ';
    while (isspace(last_char))
        last_char = getchar();

    if (isalpha(last_char))
    {
        identifier_str = last_char;
        while (isalnum((last_char = getchar())))
            identifier_str += last_char;

        if (identifier_str == "def")
            return Def;
        if (identifier_str == "extern")
            return Extern;
        return Identifier;
    }

    if (isdigit(last_char) || last_char == '.')
    {
        std::string num_str;
        do
        {
            num_str += last_char;
            last_char = getchar();
        } while (isdigit(last_char) || last_char == '.');

        num_val = strtod(num_str.c_str(), 0);
        return Number;
    }

    if (last_char == '#')
    {
        do
            last_char = getchar();
        while (last_char != EOF && last_char != '\n' && last_char != '\r');
        if (last_char != EOF)
            return gettok();
    }

    if (last_char == EOF)
        return Eof;
    int this_char = last_char;
    last_char = getchar();
    return this_char;
}