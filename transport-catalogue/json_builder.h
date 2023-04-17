#pragma once

#include "json.h"

#include <string>
#include <vector>
#include <optional>

namespace json{

class Builder;

class Builder{
public:

class DictContext;
class ArrayContext;
class KeyContext;

class BaseContext {
public:
    BaseContext(Builder& builder);
    KeyContext Key(std::string key);
    Builder& Value(Node::Value val);
    DictContext StartDict();
    ArrayContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    Node Build();
 
protected:
    Builder& builder_;
};

class DictContext final : public BaseContext{
public:
    DictContext(Builder& build);

    Builder& Value(Node::Value val) = delete;

    DictContext StartDict() = delete;

    ArrayContext StartArray() = delete;

    Builder& EndArray() = delete;

    Node Build() = delete;
};

class ArrayContext final : public BaseContext{
public:
    ArrayContext(Builder& buld);

    ArrayContext Value(Node::Value val);

    KeyContext Key(std::string key) = delete;

    Builder& EndDict() = delete;

    Node Build() = delete;
};

class KeyContext final : public BaseContext{
public:
    KeyContext(Builder& build);

    DictContext Value(Node::Value val);

    KeyContext Key(std::string key) = delete;

    Builder& EndDict() = delete;

    Builder& EndArray() = delete;

    Node Build() = delete;
};

    Builder();

    Builder(Builder& build);

    Builder& Value(Node::Value val);

    DictContext StartDict();

    KeyContext Key(std::string key);

    Builder& EndDict();

    ArrayContext StartArray();

    Builder& EndArray();

    std::vector<Node*>& GetStack();

    std::optional<std::string>& GetKey();

    Node Build();

protected:
    std::optional<std::string> key_;
    std::vector<Node*> stack;
    Node res;
};

} // namespace json