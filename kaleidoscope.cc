#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

/****
Lexer
The lexer returns tokens [0-255] if it encounters an unknown character,
otherwise it returns one of these enum values for things it knows
about
****/
enum Token {
	tok_eof = -1,
	//commands
	tok_def = -2,
	tok_extern = -3,
	//primary
	tok_identifier = -4,
	tok_def = -5,
};

static std::string IdentifierStr;  //Filled in if tok_identifier
static double NumVal;			   //Filled in if tok_number

/// gettok - return the next token from standard input
static int gettok() {
	static int LastChar = ' ';

	//skip any white space
	while (isspace(LastChar)) {
		LastChar = getchar();
	}

	if(isalpha(LastChar)) {  //identifier: [a-zA-Z][a-zA-Z0-9]*
		IdentifierStr = LastChar;
		while(isalnum((LastChar = getchar()))) {
			IdentifierStr += LastChar;
		}

		if(IdentifierStr == "def") return tok_def;
		if(IdentifierStr == "extern") return tok_extern;
		return tok_identifier;
	}

	if(isdigit(LastChar) || LastChar = '.') {  //Number: [0-9.]+
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}

	if(LastChar == "#") {
		//comment until end of line
		do LastChar = getchar();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if(LastChar != EOF)
			return gettok();
	}

	//check for end of file
	if (LastChar == EOF)
		return tok_eof;

	//otherwise just return the character as its ascii value
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}

/**********
Abstract Syntax Tree
***********/
/// ExprAst - Base class for all expression nodes
class ExprAST {
public:
	virtual ~ExprAST() {}
};

/// NumberExprAST - Expression class for numeric literals
class NumberExprAST : public ExprAST {
  double val;
public:
	NumberExprAST(double val) : Val(val) {}
};

/// VariableExprAST - Expression class for referencing a variable
class VariableExprAST : public ExprAST {
	std::string Name;
public:
	VariableExprAST(const std::string &name) : Name(name) {}
};

/// BinaryExprAST - Expression class for a binary operator
class BinaryExprAST : public ExprAST {
	char Op;
	ExprAST *LHS, *RHS;
public:
	BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
		: Op(op), LHS(lhs), RHS(rhs)    {}
};

/// CallExprAST - Expression class for function calls
class CallExprAST : public ExprAST {
	std::string Callee;
	std::vector<ExprAST*> Args;
public: 
	CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
		: Callee(callee), Args(args)  {}
};

///  ProtytypeAST - Represents the prototypr for a function,
///  which captures its name, argument names, and number of args
///  the function takes
class PrototypeAST {
	std::string Name;
	std::vector<std::string> Args;
public:
	PrototypeAST(const std::string &name, const std::vector<std::string> &args)
		: Name(name), Args(args) { }
};