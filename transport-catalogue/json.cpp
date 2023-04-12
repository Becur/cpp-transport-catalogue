#include "json.h"

#include <cmath>

using namespace std;

namespace json {

namespace load {

using json::Node;
using json::Dict;
using json::Array;

Node Value(istream& input);

Node Vector(istream& input) {
    Array result;
    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(Value(input));
    }
    if(c != ']'){
        throw ParsingError("Lose ']'"s);
    }

    return Node(move(result));
}

Node Num(istream& input) {
    string num = ""s;
    char ch;
    input >> ch;
    if(ch == '-'){
        num += move(ch);
        input >> ch;
    }
    if(ch == '0'){
        num += move(ch);
        input >> ch;
    }
    else{
        while((ch < 58) && (ch > 47)){
            num += move(ch);
            input >> ch;
            if(input.eof()){
                return Node(std::stoi(num));
            }
        }
    }
    bool is_double = false;
    if(ch == '.'){
        is_double = true;
        num += move(ch);
        input >> ch;
        while((ch < 58) && (ch > 47)){
            num += move(ch);
            input >> ch;
            if(input.eof()){
                return Node(std::stod(num));
            }
        }
    }
    std::string exponent = ""s;
    if((ch == 'e') || (ch == 'E')){
        input >> ch;
        if((ch == '+') || (ch == '-')){
            exponent += move(ch);
            input >> ch;
        }
        while((ch < 58) && (ch > 47)){
            exponent += move(ch);
            input >> ch;
            if(input.eof()){
                break;
            }
        }
    }
    if((ch > 57) || (ch < 48)){
        input.putback(ch);
    }
    if((is_double) || (!exponent.empty())){
        return Node(std::stod(move(num)) * 
        ((exponent.empty()) ? 1 
        : std::pow(10, std::stoi(move(exponent)))));
    }
    else{
        return Node(std::stoi(move(num)));
    }

}

Node String(istream& input) {
    string line = ""s;
    char last_ch, ch;
    last_ch = input.get();
    if(last_ch == '"'){
        return Node(move(line));
    }
    while(true){
        ch = input.get();
        if(input.eof()){
            throw ParsingError("Lose '\"'");
        }
        if((ch == '"') && (last_ch != '\\')){
            line += last_ch;
            break;
        }
        else{
            if(last_ch == '\\'){
               switch(ch){
                    case 'n': line += "\n"s; break;
                    case 'r': line += "\r"s; break;
                    case '"': line += "\""s; break;
                    case 't': line += "\t"s; break;
                    case '\\': line += "\\"s; break;
                    default: line += last_ch + ch; break;
                }
                last_ch = input.get();
                if(last_ch == '"'){
                    break;
                }
            }
            else{
                line += last_ch;
                last_ch = std::move(ch);
            }
        }
    }
    return Node(move(line));
}

Node Map(istream& input) {
    Dict result;
    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = String(input).AsString();
        input >> c;
        result.insert({move(key), Value(input)});
    }
    if(c != '}'){
        throw ParsingError("Lose '}'"s);
    }

    return Node(move(result));
}

Node Bool(istream& input){
    char ch;
    input >> ch;
    const string boolean =  (ch == 't') ? "true"s : "false"s;
    for(size_t i = 1; i < boolean.size(); ++i){
        input >> ch;
        if(ch != boolean[i]){
            throw ParsingError("Error bool");
        }
    }
    return Node(boolean.size() == 4);
}

Node Null(istream& input){
    char ch;
    const string nul = "null"s;
    for(int i = 0; i < 4; ++i){
        ch = input.get();
        if(ch != nul[i]){
            throw ParsingError("Error null");
        }
    }
    return Node(nullptr);
}

Node Value(istream& input) {
    char c;
    input >> c;
    while((c == ' ') || (c == '\n') 
    || (c == '\t') || (c == '\r')){
        input >> c;
    }
    if (c == '[') {
        return Vector(input);
    } else if (c == '{') {
        return Map(input);
    } else if (c == '"') {
        return String(input);
    } else if((c == 'f') || (c == 't')){
        input.putback(c);
        return Bool(input);
    } else if(c == 'n'){
        input.putback(c);
        return Null(input);
    }
     else if ((c == '-') || ((c < 58) && (c > 47))){
        input.putback(c);
        return Num(input);
    }
    else{
        throw ParsingError("Error");
    }
}

}  // namespace load

Node::Node(Array array)
    : value(move(array)) {
}

Node::Node(Dict map)
    : value(move(map)) {
}

Node::Node(int num)
    : value(num) {
}

Node::Node(string line)
    : value(move(line)) {
}

Node::Node(bool boolean)
    : value(boolean) {
}

Node::Node(double num)
    : value(num) {
}

Node::Node(nullptr_t)
    : value(nullptr){

}

const Array& Node::AsArray() const {
    if(IsArray()){
        return get<Array>(value);
     }
     else{
        throw logic_error("value type is not Array");
    }
}

const Dict& Node::AsMap() const {
    if(IsMap()){
        return get<Dict>(value);
     }
     else{
        throw logic_error("value type is not Map");
    }
}

int Node::AsInt() const {
    if(IsInt()){
        return get<int>(value);
     }
     else{
        throw logic_error("value type is not Int");
    }
}

const string& Node::AsString() const {
    if(IsString()){
        return get<string>(value);
     }
     else{
        throw logic_error("value type is not String");
    }
}

bool Node::AsBool() const{
    if(IsBool()){
        return get<bool>(value);
     }
     else{
        throw logic_error("value type is not Bool");
    }
}

double Node::AsDouble() const{
    if(IsPureDouble()){
        return get<double>(value);
    }
    else if(IsInt()){
        return get<int>(value);
     }
     else {
        throw logic_error("value type is not Int or Double");
    }
}

bool Node::IsInt() const{
    return holds_alternative<int>(value);
}

bool Node::IsDouble() const{
    return IsInt() || holds_alternative<double>(value);
}

bool Node::IsPureDouble() const {
    return holds_alternative<double>(value);
}

bool Node::IsBool() const{
    return holds_alternative<bool>(value);
}

bool Node::IsString() const{
    return holds_alternative<string>(value);
}

bool Node::IsNull() const{
    return holds_alternative<nullptr_t>(value);
}

bool Node::IsArray() const{
    return holds_alternative<Array>(value);
}

bool Node::IsMap() const{
    return holds_alternative<Dict>(value);
}

bool Node::operator==(const Node& other) const{
    return value == other.value;
}

bool Node::operator!=(const Node& other) const{
    return !(*this == other);
}

const auto& Node::GetValue() const{
    return value;
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& other) const{
    return GetRoot() == other.GetRoot();
}

bool Document::operator!=(const Document& other) const{
    return !(*this == other);
}

Document Load(istream& input) {
    return Document{load::Value(input)};
}

namespace detail{

using namespace std::literals;

void PrintText(std::ostream& output, const std::string& line){
    output << "\""sv;
    for(size_t i = 0; i < line.size(); ++i){
        switch(line[i]){
            case '\n': output << "\\n"sv; break;
            case '\r': output << "\\r"sv; break;
            case '\"': output << "\\\""sv; break;
            case '\t': output << "\t"sv; break;
            case '\\': output << "\\\\"sv; break;
            default: output << line[i]; break;
        }
    }
    output << "\""sv;
}

struct PrinterNode{
    std::ostream& output;

    void operator()(const nullptr_t){
        output << "null"sv;
    }
    void operator()(const int num){
        output << num;
    }
    void operator()(const double num){
        output << num;
    }
    void operator()(const std::string& line){
        PrintText(output, line);
    }
    void operator()(const bool boolean){
        output << ((boolean) ? "true"sv : "false"sv);
    }
    void operator()(const Array& array){
        output << "["sv;
        if(!array.empty()){
            std::visit(PrinterNode{output}, array[0].GetValue());
        }
        for(size_t i = 1; i < array.size(); ++i){
            output << ", "sv;
            std::visit(PrinterNode{output}, array[i].GetValue());
        }
        output << "]"sv;
    }
    void operator()(const Dict& map){
        output << "{"sv;
        bool is_first = true;
        for(const auto& [key, node] : map){
            if(is_first){
                is_first = false;
            }
            else{
                output << ", "sv;
            }
            output << "\""sv << key << "\": ";
            std::visit(PrinterNode{output}, node.GetValue());
        }
        output << "}"sv;
    }
};

} // namespace detail

void Print(const Document& doc, std::ostream& output) {
    std::visit(detail::PrinterNode{output}, doc.GetRoot().GetValue());
}

}  // namespace json