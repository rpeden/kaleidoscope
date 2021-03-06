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
	tok_number = -5,
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

	if(isdigit(LastChar) || LastChar == '.') {  //Number: [0-9.]+
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}

	if(LastChar == '#') {
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
  double Val;
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

class FunctionAST {
	PrototypeAST *Proto;
	ExprAST *Body;
public:
	FunctionAST(PrototypeAST *proto, ExprAST *body)
		: Proto(proto), Body(body) {}
};

/*****************
Parser
*****************/
static ExprAST *ParseExpression();
/// CurTok/getNextToken - provide a simple token buffer. CurTok is the current
/// token the parser is looking at. getNextToken reads another token from the 
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() {
	return CurTok = gettok();
}

/// helper functions for error handling
ExprAST *Error(const char *Str) { 
	fprintf(stderr, "Error: %s\n", Str);
	return 0;
}

PrototypeAST *ErrorP(const char *Str) {
	Error(Str);
	return 0;
}

FunctionAST *ErrorF(const char *Str) {
	Error(Str);
	return 0;
}

/// numberexpr ::= number
static ExprAST *ParseNumberExpr() {
	ExprAST *Result = new NumberExprAST(NumVal);
	getNextToken(); //consume the number
	return Result;
}

/// parenexpr ::= '(' expression ')'
static ExprAST *ParseParenExpr() {
	getNextToken(); //eat '('
	ExprAST *V = ParseExpression();
	if (!V) return 0;

	if (CurTok != ')') {
		return Error("expected ')'");
	}
	getNextToken();  //eat ')'
	return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static ExprAST *ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;

	getNextToken(); //eat identifier
	if(CurTok != '(') {
		//simple variable ref
		return new VariableExprAST(IdName);
	}

	//Call
	getNextToken();  //eat (
	std::vector<ExprAST*> Args;
	if(CurTok != ')') {
		while(1) {
			ExprAST *Arg = ParseExpression();
			if (!Arg) return 0;
			Args.push_back(Arg);

			if (CurTok == ')') break;

			if (CurTok != ',')
				return Error("Expected ')' or ',' in argument list");
			getNextToken();
		}
	}

	//Eat the )
	getNextToken();

	return new CallExprAST(IdName, Args);
}

/// primary
///  ::= identifierexpr
///  ::= numberexpr
///  ::= parenexpr
static ExprAST *ParsePrimary() {
	switch (CurTok){
		default: return Error("unknown token when expecting an expression");
		case tok_identifier: return ParseIdentifierExpr();
		case tok_number:     return ParseNumberExpr();
		case '(': 			 return ParseParenExpr();
	}
}

/// BinopPrecedence - holds the precedence for each binary operator that
/// is defined. 
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - This holds the precedence of the pending binary
/// operator token
static int GetTokPrecedence() {
	if(!isascii(CurTok)){
		return -1;
	}

	//ensure it is a declared binary operator
	int TokPrec = BinopPrecedence[CurTok];
	if (TokPrec <= 0) return -1;
	return TokPrec;
}

/// expression
/// ::= primary binoprhs
static ExprAST *ParseExpression() {
 	ExprAST *LHS = ParsePrimary();
 	if (!LHS) return 0;

 	return ParseBinOpRHS(0, LHS);
}

/// binoprhs
/// ::= ('+' primary)
static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS){
	//if this is a binop, find its precedence
	while(1) {
		int TokPrec = GetTokPrecedence()

		//if this is a binop that binds at least as tightly
		//as the current binop, consume it, otherwise
		//we're done
		if(TokPrec < ExprPrec) {
			return LHS;
		}

		//we know this is a binop
		int BinOp = CurTok;
		getNextToken(); //eat binop

		//parse the primary expression after the binary operator
		ExprAST *RHS = ParsePrimary();
		if (!RHS) return 0;

		//if binop binds less tightly with RHS than the operator after
		//RHS, let the pending operator take RHS as its LHS
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec+1, RHS);
			if (RHS == 0) return 0;
		}
		
		//Merge LHS/RHS
		LHS = new BinaryExprAST(Binop, LHS, RHS);
	} //back to top of while loop!
}

/// prototype
/// ::= id '(' id* ')'
static PrototypeAST *ParsePrototype() {
	if (CurTok != tok_identifier) {
		return ErrorP("Expected function name in prototype");
	}

	std::string FnName = IdentifierStr;
	getNextToken();

	if (CurTok != '(') {
		return ErrorP("Expected '(' in prototype");
	}

	//read the list of argument names
	std::vector<std::string> ArgNames;
	while (getNextToken == tok_identifier) {
		ArgNames.push_back(IdentifierStr);
	}
	if (CurTok != ')') {
		return ErrorP("Expected ')' in prototype");
	}

	// success. 
	getNextToken(); //eat ')'.

	return new PrototypeAST(FnName, ArgNames);
}

/// definition ::= 'def' prototype expression
static FunctionAST *ParseDefinition() {
	getNextToken(); //eat def
	PrototypeAST *Proto = ParsePrototype();
	if (Proto == 0 ) return 0;

	if(ExprAST *E = ParseExpression()) {
		return new FunctionAST(Proto, E);
	}
	return 0;
}

///external ::= 'extern' prototype
static Prototype *ParseExtern() {
	getNextToken();  //eat extern.
	return ParsePrototype();
}

/// toplevelexpr ::= expression
static FunctionAST *ParseTopLevelExpr() {
	if (ExprAST *E = ParseExpression()){
		//make an anonymous prototype.
		PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
		return new FunctionAST(Proto, E);
	}
	return 0;
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
	while(1) {
		fprintf(stderr, "ready> ");
	}
}

int main() {
	//set up standard binary operators
	//1 is lowest permissted precedence
	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20; 
	BinopPrecedence['-'] = 30;
	BinopPrecedence['*'] = 40;
}
