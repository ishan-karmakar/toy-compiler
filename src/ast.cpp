#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include "ast.hpp"
#include "lexer.hpp"

std::unique_ptr<ExprAST> parse_expr();

int cur_token;
std::unordered_map<char, int> binop_precedence;

std::unique_ptr<llvm::LLVMContext> context;
std::unique_ptr<llvm::IRBuilder<>> builder;
std::unique_ptr<llvm::Module> mod;
std::unique_ptr<llvm::FunctionPassManager> FPM;
std::unique_ptr<llvm::LoopAnalysisManager> LAM;
std::unique_ptr<llvm::FunctionAnalysisManager> FAM;
std::unique_ptr<llvm::CGSCCAnalysisManager> CGAM;
std::unique_ptr<llvm::ModuleAnalysisManager> MAM;
std::unique_ptr<llvm::PassInstrumentationCallbacks> PIC;
std::unique_ptr<llvm::StandardInstrumentations> SI;

std::unordered_map<std::string, llvm::Value *>
    named_values;

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

    get_next_token();
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

llvm::Value *NumberExprAST::codegen()
{
    return llvm::ConstantFP::get(*context, llvm::APFloat{val});
}

llvm::Value *VariableExprAST::codegen()
{
    llvm::Value *val = named_values[name];
    if (val)
        return val;
    else
    {
        std::cerr << "Unknown variable name\n";
        return nullptr;
    }
}

llvm::Value *BinaryExprAST::codegen()
{
    llvm::Value *L = LHS->codegen();
    llvm::Value *R = RHS->codegen();

    if (!L || !R)
        return nullptr;

    switch (op)
    {
    case '+':
        return builder->CreateFAdd(L, R, "add");
    case '-':
        return builder->CreateFSub(L, R, "sub");
    case '*':
        return builder->CreateFMul(L, R, "mul");
    case '<':
        L = builder->CreateFCmpULT(L, R, "cmplt");
        return builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*context), "bool");
    default:
        std::cerr << "Invalid binary operator\n";
        return nullptr;
    }
}

llvm::Value *CallExprAST::codegen()
{
    llvm::Function *callee_func = mod->getFunction(callee);
    if (!callee_func)
    {
        std::cerr << "Unknown function referenced\n";
        return nullptr;
    }

    if (callee_func->arg_size() != args.size())
    {
        std::cerr << "Incorrect number of args passed\n";
        return nullptr;
    }

    std::vector<llvm::Value *> llvm_args;
    for (const auto &arg : args)
    {
        llvm_args.push_back(arg->codegen());
        if (!llvm_args.back())
            return nullptr;
    }
    return builder->CreateCall(callee_func, llvm_args, "call");
}

llvm::Function *PrototypeAST::codegen()
{
    std::vector<llvm::Type *> doubles{args.size(), llvm::Type::getDoubleTy(*context)};
    llvm::FunctionType *FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(*context), doubles, false);
    llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, name, *mod);
    int i = 0;
    for (auto &arg : F->args())
        arg.setName(args[i++]);
    return F;
}

llvm::Function *FunctionAST::codegen()
{
    llvm::Function *func = mod->getFunction(proto->name);

    if (!func)
        func = proto->codegen();

    if (!func)
        return nullptr;

    if (!func->empty())
    {
        std::cerr << "Function cannot be redefined\n";
        return nullptr;
    }

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(BB);

    named_values.clear();
    for (auto &arg : func->args())
        named_values[std::string{arg.getName()}] = &arg;

    if (llvm::Value *ret = body->codegen())
    {
        builder->CreateRet(ret);
        llvm::verifyFunction(*func);

        FPM->run(*func, *FAM);
        return func;
    }

    func->eraseFromParent();
    return nullptr;
}

void init()
{
    context = std::make_unique<llvm::LLVMContext>();
    mod = std::make_unique<llvm::Module>("KaldeiscopeJIT", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
    FPM = std::make_unique<llvm::FunctionPassManager>();
    LAM = std::make_unique<llvm::LoopAnalysisManager>();
    FAM = std::make_unique<llvm::FunctionAnalysisManager>();
    CGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
    MAM = std::make_unique<llvm::ModuleAnalysisManager>();
    PIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
    SI = std::make_unique<llvm::StandardInstrumentations>(*context, true);
    SI->registerCallbacks(*PIC, MAM.get());

    FPM->addPass(llvm::InstCombinePass{});
    FPM->addPass(llvm::ReassociatePass{});
    FPM->addPass(llvm::GVNPass{});
    FPM->addPass(llvm::SimplifyCFGPass{});

    llvm::PassBuilder PB;
    PB.registerModuleAnalyses(*MAM);
    PB.registerFunctionAnalyses(*FAM);
    PB.crossRegisterProxies(*LAM, *FAM, *CGAM, *MAM);
}

void loop()
{
    while (true)
    {
        switch (cur_token)
        {
        case Eof:
            return;
        case ';':
            get_next_token();
            break;
        case Def:
            parse_definition()->codegen()->print(llvm::outs());
            break;
        case Extern:
            parse_extern()->codegen()->print(llvm::outs());
            break;
        default:
            parse_toplevel_expr()->codegen()->print(llvm::outs());
            break;
        }
    }
}