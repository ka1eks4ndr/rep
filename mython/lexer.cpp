#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <cctype>
#include <set>
#include <utility>
#include <iostream>
#include <sstream>

using namespace std;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

namespace parse {
bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;
    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

        return os << "Unknown token :("sv;
    }


    // если литерал соответсвует ключевому слову, 
std::pair<bool, int> Lexer::IsKeyWord(std::string_view word) {
    static std::vector<std::string> key_words{ "class", "return", "if",
                                                "else", "def", "print",
                                                "or", "None", "and",
                                                "not", "True", "False" };
    auto result = std::find(key_words.begin(), key_words.end(), word);
    if (result != key_words.end()) {
        return { true,std::distance(key_words.begin(),result) };
    }
    else {
        return { false, 100500 };
    }
}

Token Lexer::GetKeyWords(int KeyWord) {
    // обязательно сопоставлять с static std::vector<std::string> key_words
    // в методе std::pair<bool,int> Lexer::IsKeyWord (std::string_view word)
    switch (KeyWord) {
    case 0:
        return token_type::Class{};
    case 1:
        return token_type::Return{};
    case 2:
        return token_type::If{};
    case 3:
        return token_type::Else{};
    case 4:
        return token_type::Def{};
    case 5:
        return token_type::Print{};
    case 6:
        return token_type::Or{};
    case 7:
        return token_type::None{};
    case 8:
        return token_type::And{};
    case 9:
        return token_type::Not{};
    case 10:
        return token_type::True{};
    case 11:
        return token_type::False{};
    default:
        throw logic_error("GetKeyWords = bad input");
    }
}


bool Lexer::TokenChar(char c) {
    std::set<char> token_char{ '+','-','*','/','=','>','<','!',':','(',')','.',',' };
    return token_char.count(c);
}

Token Lexer::ParserComparison() {
    char c;
    char second_symbol;
    input_ >> c;
    input_ >> second_symbol;

    switch (c) {
    case '>':
        return token_type::GreaterOrEq{};
        break;
    case '<':
        return token_type::LessOrEq{};
    case '=':
        return token_type::Eq{};
    case '!':
        return token_type::NotEq{};
    default:
        throw logic_error("ParserComparison = bad input");
    }
}

Token Lexer::ParserLiteral() {
    std::string s;
    while (std::isalpha(input_.peek()) || input_.peek() == '_' || std::isdigit(input_.peek())) {
        s.push_back(static_cast<char>(input_.get()));
    }
    auto keyword = IsKeyWord(s);
    if (keyword.first) {
        return GetKeyWords(keyword.second);
    }

    return token_type::Id{ s };
}

Token Lexer::ParserDigit() {

    string str;
    while (std::isdigit(input_.peek())) {
        str.push_back(input_.get());
    }
    return token_type::Number{ std::stoi(str) };
}

Token Lexer::ParserGeneral() {
    string str;
    char c;
    while (input_ >> c && (c != ' ' && !TokenChar(c))) {
        str.push_back(c);
    }
    if (TokenChar(c)) {
        input_.putback(c);
    }

    return token_type::Id{ str };
}

Token Lexer::ParserString(char quotes) {
    Token token;
    auto it = std::istreambuf_iterator<char>(input_);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == quotes) {
            ++it;
            break;
        }
        else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case '\'':
                s.push_back('\'');
                break;
            case '"':
                s.push_back('"');
                break;
            default:
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        }
        else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        }
        else {
            s.push_back(ch);
        }
        ++it;
    }
    return token = token_type::String{ s };
}


Token Lexer::Parser() {
    char c;
    if (dedent_debt) {
        --dedent_debt;
        --indent_;
        return token_type::Dedent{};
    }
    if (c = input_.peek(); c == '\n' && new_line_) {
        input_.get();
        return Parser();
    }
    if (c = input_.peek(); c == '#') {
        std::stringbuf dyn;
        input_.get(dyn, '\n');
        return Parser();
    }
    if (c = input_.peek(); c != ' ' && new_line_ && indent_ > 0) {
        dedent_debt = indent_ - 1;
        --indent_;
        return token_type::Dedent{};
    }
    if (c = input_.get(); c == std::istream::traits_type::eof()) {
        if (!new_line_) {
            new_line_ = true;
            return token_type::Newline{};
        }
        return token_type::Eof{};
    }
    if (c == ' ' && new_line_) {
        new_line_ = false;
        int current_spaces = 1;
        while (input_.peek() == ' ') {
            c = input_.get();
            ++current_spaces;
        }
        if (input_.peek() == '\n') {
            input_.get();
            return Parser();
        }
        if ((current_spaces / 2 - indent_) == 1) {
            ++indent_;
            return token_type::Indent{};
        }
        else if ((current_spaces / 2 - indent_) < 0) {
            dedent_debt = indent_ - 1 - current_spaces / 2;
            --indent_;
            return token_type::Dedent{};
        }
        else if ((current_spaces / 2 - indent_) == 0) {
            return Parser();
        }
        else {
            throw logic_error("Wrong Indent");
        }
    }
    if (c == ' ') {
        input_ >> c;
        input_.putback(c);
        return Parser();

    }
    if (c == '"' || c == '\'') {
        new_line_ = false;
        return ParserString(c);
    }
    else if (std::isdigit(c)) {
        new_line_ = false;
        input_.putback(c);
        return ParserDigit();
    }
    else if (std::isalpha(c) || c == '_') {
        new_line_ = false;
        input_.putback(c);
        return ParserLiteral();
    }
    else if (TokenChar(c)) {
        new_line_ = false;
        if (input_.peek() == '=' && (c == '!' || c == '>' || c == '<' || c == '=')) {
            input_.putback(c);
            return ParserComparison();
        }
        else {
            return token_type::Char{ c };

        }
    }
    else if (c == '\n' && new_line_) {
        return Parser();

    }
    else if (c == '\n' && !new_line_) {
        new_line_ = true;
        return token_type::Newline{};
    }
    else {
        new_line_ = false;
        input_.putback(c);
        return ParserGeneral();

    }
}


Lexer::Lexer(std::istream& input) :input_(input) {
    vtoken_.push_back(Parser());
}

const Token& Lexer::CurrentToken() const {
    return vtoken_.back();
}

Token Lexer::NextToken() {
    if (vtoken_.back() == token_type::Eof{}) {
        return token_type::Eof{};
    }
    else {
        vtoken_.push_back(Parser());
        return vtoken_.back();
    }
}

}  // namespace parse