#include "ast.hpp"

int main()
{
    binop_precedence['<'] = 10;
    binop_precedence['+'] = 20;
    binop_precedence['-'] = 20;
    binop_precedence['*'] = 40;

    get_next_token();
    loop();

    return 0;
}