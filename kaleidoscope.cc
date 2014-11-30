
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