#pragma once
#include <string>

enum Token
{
    Eof = -1,
    Def = -2,
    Extern = -3,
    Identifier = -4,
    Number = -5
};

static std::string identifier_str;
static double num_val;

int gettok();