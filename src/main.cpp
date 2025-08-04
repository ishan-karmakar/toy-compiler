#include "ast.hpp"
#include <llvm/IR/Module.h>
#include <iostream>

extern std::unique_ptr<llvm::Module> mod;

int main()
{
    binop_precedence['<'] = 10;
    binop_precedence['+'] = 20;
    binop_precedence['-'] = 20;
    binop_precedence['*'] = 40;

    get_next_token();
    init();
    loop();
    mod->print(llvm::outs(), nullptr);

    return 0;
}