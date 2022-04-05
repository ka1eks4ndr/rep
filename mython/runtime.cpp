#include "runtime.h"

#include <algorithm>
#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
    if ( auto ptr = object.TryAs<String>()) {
        return ptr->GetValue().size() != 0;
    } else if (auto ptr = object.TryAs<Bool>()) {
        return ptr->GetValue();
    } else if (auto ptr = object.TryAs<Number>()) {
        return ptr->GetValue() != 0;
    }
    return false;
}

void ClassInstance::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    if (HasMethod( "__str__",0)) {
        auto oh = Call( "__str__",{} , context);
        oh->Print(os,context);
    } else {
        os << this;
    }

}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    if (auto ptr_metod = cls_.GetMethod(method)) {
        if (ptr_metod->formal_params.size() == argument_count) {
            return true;
        }
    }
    return false;
}

Closure& ClassInstance::Fields() {
    return closure_;
}

const Closure& ClassInstance::Fields() const {
     return const_cast<Closure&>(closure_);
}

ClassInstance::ClassInstance(const Class& cls) :
                cls_(cls) {
}

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                [[maybe_unused]] Context& context) {
    if (HasMethod(method,actual_args.size())) {
        auto ptr_metod = cls_.GetMethod(method);
        Closure closure;
        auto self = ObjectHolder::Share(*this);
        closure["self"]= self;
        for (size_t i = 0; i < ptr_metod->formal_params.size(); ++i) {
            closure[ptr_metod->formal_params[i]] = actual_args[i];
        }
        return cls_.GetMethod(method)->body->Execute(closure,context);
    } else {
        throw std::runtime_error("no method"+method);
    }
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent) :
                name_(std::move(name)),
                methods_(std::move(methods)),
                parent_(std::move(parent)) {
    
}

const Method* Class::GetMethod(const std::string& name) const {
    auto element = std::find_if(methods_.begin(), methods_.end(),[name](const Method& method ){
                                            return method.name == name;
    });
    if (element == methods_.end()) {
        if (parent_) {
            return parent_->GetMethod(name);
        } else {
            return nullptr;
        } 
    } else {
        return &(*element);
    }
    
}

[[nodiscard]] const std::string& Class::GetName() const { // inline delete
    return name_;
}

void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
    
    os << "Class " << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
   auto lhs_ptr_str = lhs.TryAs<String>();
   auto lhs_ptr_bool = lhs.TryAs<Bool>();
   auto lhs_ptr_number = lhs.TryAs<Number>();
   auto lhs_ptr_class_instance = lhs.TryAs<ClassInstance>();
   auto rhs_ptr_str = rhs.TryAs<String>();
   auto rhs_ptr_bool = rhs.TryAs<Bool>();
   auto rhs_ptr_number = rhs.TryAs<Number>();
   auto rhs_ptr_class_instance = rhs.TryAs<ClassInstance>();
   if (lhs_ptr_str && rhs_ptr_str) {
       return lhs_ptr_str->GetValue() == rhs_ptr_str->GetValue();
   } else if (lhs_ptr_bool && rhs_ptr_bool) {
       return lhs_ptr_bool->GetValue() == rhs_ptr_bool->GetValue();
   } else if (lhs_ptr_number && rhs_ptr_number) {
       return lhs_ptr_number->GetValue() == rhs_ptr_number->GetValue();
   } else if ((lhs_ptr_class_instance && rhs_ptr_class_instance) &&
                lhs_ptr_class_instance->HasMethod("__eq__"s,1)) {
        return IsTrue(lhs_ptr_class_instance->Call("__eq__"s,{rhs},context));
   } else if ((!lhs) && (!rhs)) {
       return true;
   }  
    throw std::runtime_error("Cannot compare objects for Equal");
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    auto lhs_ptr_str = lhs.TryAs<String>();
    auto lhs_ptr_bool = lhs.TryAs<Bool>();
    auto lhs_ptr_number = lhs.TryAs<Number>();
    auto lhs_ptr_class_instance = lhs.TryAs<ClassInstance>();
    auto rhs_ptr_str = rhs.TryAs<String>();
    auto rhs_ptr_bool = rhs.TryAs<Bool>();
    auto rhs_ptr_number = rhs.TryAs<Number>();
    auto rhs_ptr_class_instance = rhs.TryAs<ClassInstance>();
    if (lhs_ptr_str && rhs_ptr_str) {
        return lhs_ptr_str->GetValue() < rhs_ptr_str->GetValue();
    } else if (lhs_ptr_bool && rhs_ptr_bool) {
        return lhs_ptr_bool->GetValue() < rhs_ptr_bool->GetValue();
    } else if (lhs_ptr_number && rhs_ptr_number) {
        return lhs_ptr_number->GetValue() < rhs_ptr_number->GetValue();
    } else if ((lhs_ptr_class_instance && rhs_ptr_class_instance) &&
                lhs_ptr_class_instance->HasMethod("__lt__"s,1)) {
        return IsTrue(lhs_ptr_class_instance->Call("__lt__"s,{rhs},context));
    }  
        throw std::runtime_error("Cannot compare objects for Less");
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !(Equal(lhs, rhs, context));
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !(Less(lhs,rhs,context)) && !(Equal(lhs,rhs,context));
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Less(lhs,rhs,context) || Equal(lhs,rhs,context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !(Less(lhs,rhs,context));
}

}  // namespace runtime