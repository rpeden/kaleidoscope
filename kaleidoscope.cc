
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
}