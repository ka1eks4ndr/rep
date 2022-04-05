#include "json_builder.h"

#include "algorithm"

namespace json
{
void Builder::TryMetod () {
    if ((!(root_.IsNull()))&&nodes_stack_.empty()) {
        throw std::logic_error ("attempting to call a method when the object is complete");
    }
}

Builder& Builder::Value(Node::Value value) {
    return ValueGeneric(value);
}

Builder& Builder::ValueGeneric(Node::Value value) {
    TryMetod();
    Node n_value(std::move(value));
    
    if (root_.IsNull()) {
        root_=n_value;
        
    } else if (nodes_stack_.back()->IsNull()){
        if ((nodes_stack_[nodes_stack_.size()-2])->IsDict()) {
            *nodes_stack_.back()=n_value;
            nodes_stack_.pop_back();
        } 
    } else if (nodes_stack_.back()->IsArray()) {
        Array& array=const_cast<Array&>((nodes_stack_.back())->AsArray());
        array.push_back(n_value);
        
    } else {
        throw std::logic_error ("wrong context for start array");
    }
    return *this;
}

KeyItemContext& Builder::Key(std::string value) {
    TryMetod();
    if (!nodes_stack_.back()->IsDict()) {
        throw std::logic_error ("wrong context for key");
    }
    Dict& dict=const_cast<Dict&>((nodes_stack_.back())->AsDict());
    nodes_stack_.emplace_back(&(dict[value]));
    return static_cast<KeyItemContext&>(*this);
}

ArrayItemContext& Builder::StartArray() {
    TryMetod();
    Array arr;
    if (root_.IsNull()) {
        root_=arr;
        nodes_stack_.emplace_back(&root_);
    } else if (nodes_stack_.back()->IsNull()){
        if ((nodes_stack_[nodes_stack_.size()-2])->IsDict()) {
            *nodes_stack_.back()=arr;
        } 
    } else if (nodes_stack_.back()->IsArray()) {
        Array& array=const_cast<Array&>((nodes_stack_.back())->AsArray());
        //array.push_back(arr);
        nodes_stack_.emplace_back(&((array.emplace_back(arr))));
    } else {
        throw std::logic_error ("wrong context for start array");
    }
    return static_cast<ArrayItemContext&>(*this);
}

Builder& Builder::EndArray() {
    TryMetod();
    if (nodes_stack_.back()->IsArray()) {
        nodes_stack_.pop_back();
    } else {
        throw std::logic_error ("wrong context for end array");
    }
    
    return *this;
    
}

DictItemContext& Builder::StartDict() {
    return StartDictGeneric();
}

DictItemContext& Builder::StartDictGeneric() {
   TryMetod();
    Dict dict;
    if (root_.IsNull()) {
        root_=dict;
        nodes_stack_.emplace_back(&root_);
    } else if (nodes_stack_.back()->IsNull()){
        if ((nodes_stack_[nodes_stack_.size()-2])->IsDict()) {
            *nodes_stack_.back()=dict;
        } 
    } else if (nodes_stack_.back()->IsArray()) {
        Array& array=const_cast<Array&>((nodes_stack_.back())->AsArray());
        //array.push_back(dict);
        nodes_stack_.emplace_back(&((array.emplace_back(dict))));
    } else {
        throw std::logic_error ("wrong context for start array");
    }
    return static_cast<DictItemContext&>(*this);
}

Builder& Builder::EndDict() {
    TryMetod();
    if (nodes_stack_.back()->IsDict()) {
        nodes_stack_.pop_back();
    } else {
        throw std::logic_error ("wrong context for stop dict");
    }
    return *this;
}

json::Node Builder::Build() {
    
    if (!nodes_stack_.empty()) {
        throw std::logic_error("builder not complete");
    } else if (root_.IsNull()){
        throw std::logic_error("builder empty");
    }
    //root_=*nodes_stack_.back();
    return root_;
}

//----------------DictItemContext-----------------

DictItemContext& DictItemContext::Value(Node::Value value) {
    
    return static_cast<DictItemContext&>(ValueGeneric(value));
}

//----------------ArrayItemContext-----------------

ArrayItemContext& ArrayItemContext::Value(Node::Value value) {
   
    return static_cast<ArrayItemContext&>(ValueGeneric(value));
}

//----------------ArrayItemContext-----------------

KeyDictContext& KeyItemContext::Value(Node::Value value) {
    
    return static_cast<KeyDictContext&>(ValueGeneric(value));
}

} // namespace json
