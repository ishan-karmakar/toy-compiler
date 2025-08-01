#pragma once
#include <string>
#include <memory>
#include <vector>

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
    BinaryExprAST(char _op, std::unique_ptr<ExprAST> _LHS, std::unique_ptr<ExprAST> _RHS) : op{_op}, LHS{_LHS}, RHS{_RHS} {}
};

class CallExprAST : public ExprAST
{
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;

public:
    CallExprAST(const std::string _callee, std::vector<std::unique_ptr<ExprAST>> args) : callee{_callee}, args{_args} {}
};

class PrototypeAST
{
    std::string name;
    std::vector<std::string> args;

public:
    PrototypeAST(const std::string _name, std::vector<std::string> _args) : name{_name}, args{_args} {}
};