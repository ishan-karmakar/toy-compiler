#include <iostream>
#include "ast.hpp"
#include "lexer.hpp"

std::unique_ptr<ExprAST> parse_expr();

int cur_token;
std::unordered_map<char, int> binop_precedence;

int get_next_token()
{
    return cur_token = gettok();
}

std::unique_ptr<ExprAST> parse_num_expr()
{
    auto result = std::make_unique<NumberExprAST>(num_val);
    get_next_token();
    return result;
}

std::unique_ptr<ExprAST> parse_paren_expr()
{
    get_next_token();
    auto expr = parse_expr();
    if (!expr)
        return nullptr;

    if (cur_token != ')')
    {
        std::cerr << "Expected ')'\n";
        exit(1);
    }

    gettok();
    return expr;
}

std::unique_ptr<ExprAST> parse_identifier_expr()
{
    std::string id_name = identifier_str;
    get_next_token();

    // Simple variable ref
    if (cur_token != '(')
        return std::make_unique<VariableExprAST>(id_name);
    // Function ref
    get_next_token();
    std::vector<std::unique_ptr<ExprAST>> args;
    if (cur_token != ')')
    {
        // This function has arguments
        while (true)
        {
            if (auto arg = parse_expr())
                args.push_back(std::move(arg));
            else
                return nullptr;

            if (cur_token == ')')
                break;
            if (cur_token != ',')
            {
                std::cerr << "Expected ')' or ',' in argument list\n";
                return nullptr;
            }
            get_next_token();
        }
    }
    get_next_token();

    return std::make_unique<CallExprAST>(id_name, std::move(args));
}

std::unique_ptr<ExprAST> parse_primary()
{
    switch (cur_token)
    {
    case Identifier:
        return parse_identifier_expr();
    case Number:
        return parse_num_expr();
    case '(':
        return parse_paren_expr();
    default:
        std::cerr << "Unknown token when expecting an expression\n";
        return nullptr;
    }
}

int get_tok_precedence()
{
    if (!isascii(cur_token))
        return -1;

    int tok_prec = binop_precedence[cur_token];
    if (tok_prec <= 0)
        return -1;
    return tok_prec;
}

std::unique_ptr<ExprAST> parse_binop_rhs(int expr_prec, std::unique_ptr<ExprAST> LHS)
{
    while (true)
    {
        int tok_prec = get_tok_precedence();

        if (tok_prec < expr_prec)
            return LHS;

        int binop = cur_token;
        get_next_token();

        auto RHS = parse_primary();
        if (!RHS)
            return nullptr;

        int next_prec = get_tok_precedence();
        if (tok_prec < next_prec)
        {
            RHS = parse_binop_rhs(tok_prec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        LHS = std::make_unique<BinaryExprAST>(binop, std::move(LHS), std::move(RHS));
    }
}

std::unique_ptr<ExprAST> parse_expr()
{
    auto LHS = parse_primary();
    if (!LHS)
        return nullptr;

    return parse_binop_rhs(0, std::move(LHS));
}

std::unique_ptr<PrototypeAST> parse_prototype()
{
    if (cur_token != Identifier)
    {
        std::cerr << "Expected function name in prototype\n";
        return nullptr;
    }

    std::string func_name = identifier_str;
    get_next_token();

    if (cur_token != '(')
    {
        std::cerr << "Expected '(' in prototype\n";
        return nullptr;
    }

    std::vector<std::string> args;
    while (get_next_token() == Identifier)
        args.push_back(identifier_str);
    if (cur_token != ')')
    {
        std::cerr << "Expected ')' in prototype\n";
        return nullptr;
    }

    get_next_token();
    return std::make_unique<PrototypeAST>(func_name, std::move(args));
}

std::unique_ptr<FunctionAST> parse_definition()
{
    get_next_token();
    auto proto = parse_prototype();
    if (!proto)
        return nullptr;

    if (auto E = parse_expr())
        return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
    return nullptr;
}

std::unique_ptr<PrototypeAST> parse_extern()
{
    get_next_token();
    return parse_prototype();
}

std::unique_ptr<FunctionAST> parse_toplevel_expr()
{
    if (auto E = parse_expr())
    {
        auto proto = std::make_unique<PrototypeAST>("", std::vector<std::string>{});
        return std::make_unique<FunctionAST>(std::move(proto), std::move(E));
    }
    return nullptr;
}

void loop()
{
    while (true)
    {
        switch (cur_token)
        {
        case Eof:
            std::cout << "Parsing an EOF\n";
            return;
        case ';':
            std::cout << "Parsing a top level semicolon\n";
            get_next_token();
            break;
        case Def:
            std::cout << "Parsing a definition\n";
            parse_definition();
            break;
        case Extern:
            std::cout << "Parsing an extern\n";
            parse_extern();
            break;
        default:
            std::cout << "Parsing a top level expression\n";
            parse_toplevel_expr();
            break;
        }
    }
}