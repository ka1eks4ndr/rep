#include "statement.h"

#include <iostream>
#include <sstream>
#include <cassert>
#

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    closure[var_] = rv_->Execute(closure,context);
    return closure[var_];
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) :
            var_(std::move(var)),
            rv_(std::move(rv)) {
}

VariableValue::VariableValue(const std::string& var_name) :
            var_name_(var_name) {
    
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) : 
            dotted_ids_(std::move(dotted_ids)) {

}

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    if (!var_name_.empty()) {
        if (closure.count(var_name_)) {
            return closure[var_name_];
        } else {
            throw std::runtime_error("error VariableValue");
        }
    } else if (!dotted_ids_.empty()) {
        ObjectHolder* chain = &closure[dotted_ids_[0]];
        for (size_t i = 1; i < dotted_ids_.size(); ++i) {
            chain = &chain->TryAs<runtime::ClassInstance>()->Fields()[dotted_ids_[i]];
        }
        return *chain;
    } else {
        throw std::runtime_error("error VariableValue");
    }
}

Print::Print(const std::string& name) : 
        name_(name) {

}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return make_unique<Print>(Print(name));
    
}

Print::Print(unique_ptr<Statement> argument) :
        argument_(std::move(argument)) {
}

Print::Print(vector<unique_ptr<Statement>> args) : 
        args_(std::move(args)){
}


ObjectHolder Print::Execute(Closure& closure, Context& context) {
    if (!name_.empty()) {
        ostringstream ostr;
        auto& os = context.GetOutputStream();
        ObjectHolder oh = closure[name_];
        if (oh) {
            oh->Print(os,context);
            os << '\n';
        } else  {
            os << "None";
            os << '\n';
        }
    } else if (argument_) {
        auto& os = context.GetOutputStream();
        argument_->Execute(closure,context)->Print(os,context);
    } else if (!args_.empty()) {
        auto& os = context.GetOutputStream();
        bool first = true;
        for (const auto& expr : args_) {
            if (first) {
                ObjectHolder oh = expr->Execute(closure, context);
                if (oh) {
                    oh->Print(os,context);
                } else {
                    os << "None";
                }
                first = false;
                continue;
            }
            os << ' ';
            if (ObjectHolder oh = expr->Execute(closure,context)) {
                oh->Print(os,context);
            } else {
                os << "None";
            }
        }
        os << '\n';
    } else {
        auto& os = context.GetOutputStream();
        os << '\n';
    }
    return {};
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args) :
                    object_(std::move(object)),
                    method_(std::move(method)),
                    args_(std::move(args)) {
    
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    std::vector<ObjectHolder>params;
    for (const auto& state_params : args_) {
        params.push_back(state_params->Execute(closure,context));
    }
    return object_->Execute(closure,context).TryAs<runtime::ClassInstance>()->Call(method_,params,context);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    auto oh = this->argument_->Execute(closure,context);
    if (oh) {
        ostringstream ostream;
        oh->Print(ostream,context);
        return ObjectHolder::Own(runtime::String(ostream.str()));
    } else {
        return ObjectHolder::Own(runtime::String("None"));
    }
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_oh = GetLhs()->Execute(closure, context);
    ObjectHolder rhs_oh = GetRhs()->Execute(closure, context);
    auto lhs_oh_number = lhs_oh.TryAs<runtime::Number>();
    auto lhs_oh_string = lhs_oh.TryAs<runtime::String>();
    auto lhs_oh_class_instance = lhs_oh.TryAs<runtime::ClassInstance>();
    auto rhs_oh_number = rhs_oh.TryAs<runtime::Number>();
    auto rhs_oh_string = rhs_oh.TryAs<runtime::String>();
    auto rhs_oh_class_instance = rhs_oh.TryAs<runtime::ClassInstance>();
    if (lhs_oh_number && rhs_oh_number) {
        auto answer = lhs_oh_number->GetValue()+rhs_oh_number->GetValue();
        return ObjectHolder::Own(runtime::Number(answer));
    } else if (lhs_oh_string && rhs_oh_string) {
        return ObjectHolder::Own(runtime::String(lhs_oh_string->GetValue()+rhs_oh_string->GetValue()));
    } else if (lhs_oh_class_instance && (rhs_oh_class_instance || rhs_oh_number || rhs_oh_string)) {
        if (lhs_oh_class_instance->HasMethod("__add__",1)) {
            return lhs_oh_class_instance->Call("__add__"s,{rhs_oh},context);
        }
    }
    throw std::runtime_error("ObjectHolder Add::Execute - TT");
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_oh = GetLhs()->Execute(closure, context);
    ObjectHolder rhs_oh = GetRhs()->Execute(closure, context);
    auto lhs_oh_number = lhs_oh.TryAs<runtime::Number>();
    auto rhs_oh_number = rhs_oh.TryAs<runtime::Number>();
    if (lhs_oh_number && rhs_oh_number) {
        auto answer = lhs_oh_number->GetValue()-rhs_oh_number->GetValue();
        return ObjectHolder::Own(runtime::Number(answer));
    }
    throw std::runtime_error("ObjectHolder Sub::Execute - TT");
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_oh = GetLhs()->Execute(closure, context);
    ObjectHolder rhs_oh = GetRhs()->Execute(closure, context);
    auto lhs_oh_number = lhs_oh.TryAs<runtime::Number>();
    auto rhs_oh_number = rhs_oh.TryAs<runtime::Number>();
    if (lhs_oh_number && rhs_oh_number) {
        auto answer = lhs_oh_number->GetValue()*rhs_oh_number->GetValue();
        return ObjectHolder::Own(runtime::Number(answer));
    }
    throw std::runtime_error("ObjectHolder Mult::Execute - TT");
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_oh = GetLhs()->Execute(closure, context);
    ObjectHolder rhs_oh = GetRhs()->Execute(closure, context);
    auto lhs_oh_number = lhs_oh.TryAs<runtime::Number>();
    auto rhs_oh_number = rhs_oh.TryAs<runtime::Number>();
    if (lhs_oh_number && rhs_oh_number) {
        if (rhs_oh_number->GetValue() != 0) {
            auto answer = lhs_oh_number->GetValue()/rhs_oh_number->GetValue();
        return ObjectHolder::Own(runtime::Number(answer));
        }
    }
    throw std::runtime_error("ObjectHolder Div::Execute - TT");
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
   for (const auto& stmt : stmts_) {
       stmt->Execute(closure,context);
   }
   return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    ObjectHolder holder = statement_.get()->Execute(closure, context);
    throw holder;
    return {};
}

ClassDefinition::ClassDefinition(ObjectHolder cls) : 
            cls_(cls) {
    
}

ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context) {
    
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return closure[cls_.TryAs<runtime::Class>()->GetName()];
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv) :
                object_(object),
                field_name_(std::move(field_name)),
                rv_(std::move(rv)) {
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    return object_.Execute(closure, context).TryAs<runtime::ClassInstance>()->Fields()[field_name_] = rv_->Execute(closure, context);
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body) : 
            condition_(std::move(condition)),
            if_body_(std::move(if_body)),
            else_body_(std::move(else_body)) {
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    
    if (runtime::IsTrue(condition_->Execute(closure, context))) {
        return if_body_->Execute(closure, context);
    }
    else if (else_body_!=nullptr) {
        return else_body_->Execute(closure, context);
    } else {
        return {};
    }
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_oh = GetLhs()->Execute(closure, context);
    
    if (!(runtime::IsTrue(lhs_oh))) {
       return GetRhs()->Execute(closure, context);
    } else {
        return lhs_oh;
    }
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_oh = GetLhs()->Execute(closure, context);
    ObjectHolder rhs_oh = GetRhs()->Execute(closure, context);
    if (runtime::IsTrue(lhs_oh) && runtime::IsTrue(rhs_oh)) {
        return ObjectHolder::Own(runtime::Bool(runtime::Equal(lhs_oh, rhs_oh,context)));
    } 
    return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    if (IsTrue(argument_->Execute(closure,context))) {
        return ObjectHolder::Own(runtime::Bool(false));
    } else {
        return ObjectHolder::Own(runtime::Bool(true));
    }
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)),
    cmp_(cmp) {

}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs_oh = GetLhs()->Execute(closure, context);
    ObjectHolder rhs_oh = GetRhs()->Execute(closure, context);
   
    return runtime::ObjectHolder::Own(runtime::Bool(cmp_(lhs_oh, rhs_oh, context)));
    
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args) : 
            class__(class_),
            args_(std::move(args)){
}

NewInstance::NewInstance(const runtime::Class& class_) :
            class__(class_) {
    
}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    auto oh = ObjectHolder::Own(runtime::ClassInstance (class__));
    if (oh.TryAs<runtime::ClassInstance>()->HasMethod("__init__",args_.size())) {
        vector<runtime::ObjectHolder>oh_args_;
        for (const auto& arg_statement : args_) {
            oh_args_.push_back(arg_statement->Execute(closure,context));
        }
        oh.TryAs<runtime::ClassInstance>()->Call("__init__", oh_args_, context);
        return oh;
    } 
    return oh;
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : 
            body_(std::move(body)) {
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        body_.get()->Execute(closure, context);
    }
    catch (ObjectHolder const& holder)
    {
        return holder;
    }
    return {};
}


}  // namespace ast