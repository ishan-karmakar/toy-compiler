#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

class ExprAST
{
public:
    virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST
{
    double val;

public:
    NumberExprAST(double _val) : val{_val} {}
};

class VariableExprAST : public ExprAST
{
    std::string name;

public:
    VariableExprAST(const std::string _name) : name{_name} {}
};

class BinaryExprAST : public ExprAST
{
    char op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char _op, std::unique_ptr<ExprAST> _LHS, std::unique_ptr<ExprAST> _RHS) : op{_op}, LHS{std::move(_LHS)}, RHS{std::move(_RHS)} {}
};

class CallExprAST : public ExprAST
{
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;

public:
    CallExprAST(const std::string _callee, std::vector<std::unique_ptr<ExprAST>> _args) : callee{_callee}, args{std::move(_args)} {}
};

class PrototypeAST
{
    std::string name;
    std::vector<std::string> args;

public:
    PrototypeAST(const std::string _name, std::vector<std::string> _args) : name{_name}, args{_args} {}
};

class FunctionAST
{
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<ExprAST> body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> _proto, std::unique_ptr<ExprAST> _expr) : proto{std::move(_proto)}, body{std::move(_expr)} {}
};

int get_next_token();
void loop();

extern std::unordered_map<char, int>
    binop_precedence;