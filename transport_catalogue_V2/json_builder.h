#pragma once

#include "json.h"

namespace json 
{

class DictItemContext;
class ArrayItemContext;
class KeyItemContext;
class KeyDictContext;

class Builder {
public:
    Builder () = default;

    virtual Builder& Value(Node::Value value);

    KeyItemContext& Key(std::string value);

    ArrayItemContext& StartArray();

    DictItemContext& StartDict();

    json::Node Build();
    
    virtual ~Builder()=default;

    Builder& EndArray();
    
    Builder& EndDict();

protected:
    DictItemContext& StartDictGeneric();
    Builder& ValueGeneric(Node::Value value);

    
    
    void TryMetod();
    Node root_;
    std::vector<Node*> nodes_stack_;
private:
   

};
   
class DictItemContext : public Builder {
public:
    DictItemContext& Value(Node::Value value);
    using Builder::Key;
    using Builder::EndDict;
private:
    using Builder::StartArray;
    using Builder::StartDict;
    using Builder::Value;
    using Builder::Build;
    using Builder::EndArray;
    
};

class ArrayItemContext : public Builder {
public:
    ArrayItemContext& Value(Node::Value value);
    
private:
    using Builder::Value;
    using Builder::Build;
    using Builder::Key;
    using Builder::EndDict;
};

class KeyDictContext : public Builder {

private:
    using Builder::StartArray;
    using Builder::StartDict;
    using Builder::Value;
    using Builder::Build;
    using Builder::EndArray;
};

class KeyItemContext : public Builder {
public:
    KeyDictContext& Value(Node::Value value);
private:
    using Builder::Key;
    using Builder::Build;
    using Builder::EndArray;
    using Builder::EndDict;    
};

} // namespace json 