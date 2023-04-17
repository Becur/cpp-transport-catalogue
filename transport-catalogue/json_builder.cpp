#include "json_builder.h"

#include <variant>

namespace json{

Builder::Builder() : res(Node()){
    stack.push_back(&res);
}

Builder::Builder(Builder& build)
: key_(std::move(build.key_))
, stack(std::move(build.stack))
, res(std::move(build.res)){}

Builder& Builder::Value(Node::Value val){
    if(stack.empty()){
        throw std::logic_error("Error Value");
    }
    if(stack.back()->IsNull()){
        res = Node(std::move(val));
        stack.pop_back();
    } 
    else if(stack.back()->IsArray()){
        std::get<Array>(stack.back()->Get()).push_back(Node(std::move(val)));
    }
    else{
        throw std::logic_error("Error implace Value");
    }
    return *this;
}

Builder::DictContext Builder::StartDict(){
    if(stack.empty() || (stack.back()->IsDict() & !key_.has_value())){
        throw std::logic_error("Error StartDict");
    }
    if(stack.back()->IsNull()){
        res = Node(Dict());
    } 
    else if(stack.back()->IsArray()){
        std::get<Array>(stack.back()->Get()).push_back(Node(Dict()));
        stack.push_back(&std::get<Array>(stack.back()->Get()).back());
    }
    else if(stack.back()->IsDict()){
        if(!key_.has_value()){
            throw std::logic_error("Eror: missing key");
        }
        std::get<Dict>(stack.back()->Get()).insert({key_.value(), Node(Dict())});
        stack.push_back(&std::get<Dict>(stack.back()->Get()).at(std::move(key_.value())));
        key_.reset();
    }
    else{
        throw std::logic_error("Error Implace value");
    }
    return DictContext(*this);
}

Builder::KeyContext Builder::Key(std::string key){
    if(stack.empty() || !stack.back()->IsDict() || key_.has_value()){
        throw std::logic_error("Error Key: " + std::move(key));
    }
    key_ = std::move(key);
    return KeyContext(*this);
}

Builder& Builder::EndDict(){
    if(stack.empty() || !stack.back()->IsDict()){
        throw std::logic_error("Error EndDict");
    }
    stack.pop_back();
    return *this;
}

Builder::ArrayContext Builder::StartArray(){
    if(stack.empty() || (stack.back()->IsDict() & !key_.has_value())){
        throw std::logic_error("Error StartArray");
    }
    if(stack.back()->IsNull()){
        res = Node(Array());
    } 
    else if(stack.back()->IsArray()){
        std::get<Array>(stack.back()->Get()).push_back(Node(Array()));
        stack.push_back(&std::get<Array>(stack.back()->Get()).back());
    }
    else if(stack.back()->IsDict()){
        if(!key_.has_value()){
            throw std::logic_error("Eror: missing key");
        }
        std::get<Dict>(stack.back()->Get()).insert({key_.value(), Node(Array())});
        stack.push_back(&std::get<Dict>(stack.back()->Get()).at(std::move(key_.value())));
        key_.reset();
    }
    else{
        throw std::logic_error("Error Implace value");
    }
    return ArrayContext(*this);
}

Builder& Builder::EndArray(){
    if(stack.empty() || !stack.back()->IsArray()){
        throw std::logic_error("Error EndArray");
    }
    stack.pop_back();
    return *this;
}

Node Builder::Build(){
    if(!stack.empty()){
        throw std::logic_error("Error: Build is not complete");
    }
    return res;
}

std::vector<Node*>& Builder::GetStack(){
    return stack;
}

std::optional<std::string>& Builder::GetKey(){
    return key_;
}

Builder::BaseContext::BaseContext(Builder& builder)
: builder_(builder){}
Builder::KeyContext Builder::BaseContext::Key(std::string key){
    return builder_.Key(std::move(key));
}
Builder& Builder::BaseContext::Value(Node::Value val){
    return builder_.Value(std::move(val));
}
Builder::DictContext Builder::BaseContext::StartDict(){
    return builder_.StartDict();
}
Builder::ArrayContext Builder::BaseContext::StartArray(){
    return builder_.StartArray();
}
Builder& Builder::BaseContext::EndDict(){
    return builder_.EndDict();
}
Builder& Builder::BaseContext::EndArray(){
    return builder_.EndArray();
}
Node Builder::BaseContext::Build(){
    return builder_.Build();
}

Builder::DictContext::DictContext(Builder& build)
: BaseContext(build){}

Builder::ArrayContext::ArrayContext(Builder& build)
: BaseContext(build){}

Builder::ArrayContext Builder::ArrayContext::Value(Node::Value val){
    std::vector<Node*>& stack = builder_.GetStack();
    if(stack.empty() || !stack.back()->IsArray()){
        throw std::logic_error("Error Value");
    }
    std::get<Array>(stack.back()->Get()).push_back(Node(std::move(val)));
    return ArrayContext(builder_);
}

Builder::KeyContext::KeyContext(Builder& build)
: BaseContext(build){}

Builder::DictContext Builder::KeyContext::Value(Node::Value val){
    std::vector<Node*>& stack = builder_.GetStack();
    std::optional<std::string>& key_ = builder_.GetKey();
    if(stack.empty() || !stack.back()->IsDict()){
        throw std::logic_error("Error Value");
    }
    std::get<Dict>(stack.back()->Get()).insert({std::move(key_.value()), Node(std::move(val))});
    key_.reset();
    return DictContext(builder_);
}

} // namespace json