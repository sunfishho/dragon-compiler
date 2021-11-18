#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <variant>
#include <map>

// using namespace std;

//word will eventually be split into identifiers and keywords


enum Tag {number, word, eof, oper, assignment, enclose, punctuation, literal, error};
enum Op {logor, logand, noteq, eq, leq, geq, lt, 
         gt, lognot, plus, minus, times, divide, mod};
enum AssignmentOp {plusequals, minusequals, assignequals};
enum Enclosures {lparen, rparen, lcurly, rcurly, lsquare, rsquare};
enum Punctuation {comma, semicolon};
enum Literals {charliteral, stringliteral};
//expand on this later
enum Errors {illegalchar, missingquotes};

//  Your scanner should note illegal characters, missing quotation marks, and other lexical errors with reasonable error 
// messages. The scanner should find as many lexical errors as possible, and should be able to continue scanning after 
// errors are found. The scanner should also filter out comments and whitespace not in string and character literals.


struct Token{
    Tag tag;
    std::variant<int32_t, std::string, std::monostate, Op, AssignmentOp, Enclosures, Punctuation, Literals, Errors> value;
};

std::vector<std::pair<std::string, Token>> symbolMatcher = { {"||", {oper, logor}}, 
                                                             {"&&", {oper, logand}},
                                                             {"!=", {oper, noteq}},
                                                             {"==", {oper, eq}},
                                                             {"<=", {oper, leq}},
                                                             {">=", {oper, geq}},
                                                             {"+=", {assignment, plusequals}},
                                                             {"-=", {assignment, minusequals}},
                                                             {"<",  {oper, lt}},
                                                             {">",  {oper, gt}},
                                                             {"!",  {oper, lognot}},
                                                             {"+",  {oper, plus}},
                                                             {"-",  {oper, minus}},
                                                             {"*",  {oper, times}},
                                                             {"/",  {oper, divide}},
                                                             {"%",  {oper, mod}}, 
                                                             {"=",  {assignment, assignequals}},
                                                             {"(",  {enclose, lparen}},
                                                             {")",  {enclose, rparen}},
                                                             {"{",  {enclose, lcurly}},
                                                             {"}",  {enclose, rcurly}},
                                                             {"[",  {enclose, lsquare}},
                                                             {"]",  {enclose, rsquare}},
                                                             {",",  {punctuation, comma}},
                                                             {";",  {punctuation, semicolon}}};

//from https://stackoverflow.com/questions/62355613/stdvariant-cout-in-c
struct make_string_functor{
  std::string operator()(const std::string &x) const { return x; }
  std::string operator()(int x) const { return std::to_string(x); }
  std::string operator()(std::monostate _) const { return "EOF"; }
};

std::ostream &operator<<(std::ostream &os, const Token &token){   
    std::cout << token.tag << " " << std::visit(make_string_functor(), token.value);
    return os;
}

bool isDigit(char c){
    return (c >= '0' && c <= '9');
}

bool isLetter(char c){
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_');
}

int32_t charToDigit(char c){
    assert (isDigit(c));
    return c - '0';
}

//Given that the next token to be scanned is a number, scan the number
Token scanNumber(std::ifstream &input){
    char peek = (char) input.peek();
    assert(isDigit(peek));
    int next_num = charToDigit(peek);
    input.ignore(1);
    peek = (char) input.peek();
    while (isDigit(peek)){
        next_num = next_num * 10 + charToDigit(peek);
        input.ignore(1);
        peek = (char) input.peek();
    }
    return {number, next_num};
}

//Given that the next token to be scanned is a word, scan the word
Token scanWord(std::ifstream &input){
    char peek = (char) input.peek();
    assert(isLetter(peek));
    std::string next_word = "";
    while (isLetter(peek) || isDigit(peek)){
        next_word += peek;
        input.ignore(1);
        peek = (char) input.peek();
    }
    return {word, next_word};
}

std::string peekNCharacters(std::ifstream &input, int n){
    assert(n >= 1);
    std::string nextN = "";
    for (int i = 0; i < n; i++){
        char peek = (char) input.peek();
        nextN += peek;
        input.ignore(1);
    }
    for (int i = 0; i < n; i++){
        input.unget();
    }
    return nextN;
}

//When there's a line comment, move the ifstream to the beginning of the next line
void skipComment(std::ifstream &input){
    assert(peekNCharacters(input, 2) == "//");
    while (input.peek() != '\n' && !input.eof()){
        input.ignore(1);
    }
    input.ignore(1);
}

Token scanSymbol(std::ifstream &input){
    std::string twoChars = peekNCharacters(input, 2);
    for (int idx = 0; idx < 8; idx++){
        if (twoChars == symbolMatcher[idx].first){
            input.ignore(2);
            return symbolMatcher[idx].second;
        }
    }
    for (int idx = 8; idx < symbolMatcher.size(); idx++){
        if (twoChars.substr(0, 1) == symbolMatcher[idx].first){
            input.ignore(1);
            return symbolMatcher[idx].second;
        }
    }
    return {eof, std::monostate{}};
    //should create some mapping from operators to tag
}
//current bug: if file ends in comment, something goes wrong and the program doesn't finish.
Token scanOneToken(std::ifstream &input){
    if (input.peek() == EOF || input.eof()){
       return {eof, std::monostate{}};
    }
    char peek = (char) input.peek();
    for (; ; peek = (char) input.peek()){
        if (input.peek() == EOF || input.eof()){
            return {eof, std::monostate{}};
        }
        if (peek == ' ' || peek == '\t' || peek == '\n'){
            input.ignore(1);
            continue;
        }
        else if (isDigit(peek)){
            return scanNumber(input);
        }
        else if (isLetter(peek)){
            return scanWord(input);
        }
        else if (peekNCharacters(input, 2) == "//"){
            skipComment(input);
            return scanOneToken(input);
        }
        //need to implement some check to see we're actually finding an operator
        else{
            Token sym = scanSymbol(input);
            if (sym.tag != eof){
                return sym;
            }
            else{
                //need to treat this case (brackets, parentheses, etc.)
                input.ignore(1);
                return scanOneToken(input);
            }
        }
    }
}

std::vector<Token> scanner(std::string filename){
    std::ifstream input;
    input.open(filename);
    std::vector<Token> tokenList;
    Token nextToken = scanOneToken(input);
    while (nextToken.tag != eof){
        tokenList.push_back(nextToken);
        nextToken = scanOneToken(input);
    }
    input.close();
    return tokenList;
}


int main(){
    std::vector<Token> tokenVector = scanner("test.in");
    for (auto token : tokenVector){
        std::cout << token << std::endl;
    }
}

