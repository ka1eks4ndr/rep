#pragma once

#include <cassert>
#include <initializer_list>
#include "array_ptr.h"
#include <array>
#include <iterator>
#include <iostream>
#include <utility>


class ReserveProxyObj 
{
private:
    size_t size_to_reserve_=0;
public:
    ReserveProxyObj (size_t n)
        :size_to_reserve_(n)
        {
        } 
    ReserveProxyObj ()=default;
    size_t Get() {
        return size_to_reserve_;
    }
};
 
ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}


template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) {
        ArrayPtr<Type> numbers(size);
        storage_.swap(numbers);
        size_ = size;
        capacity_ = size;

    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) {
        ArrayPtr<Type> numbers(size, value);
        storage_.swap(numbers);
        size_ = size;
        capacity_ = size;
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        ArrayPtr<Type> numbers(init);
        storage_.swap(numbers);
        size_ = init.size();
        capacity_ = init.size();
    }

    SimpleVector(ReserveProxyObj obj) {
        SimpleVector();
        this->Reserve(obj.Get());
    }

    SimpleVector(const SimpleVector& other) {
        assert(size_ == 0);
        SimpleVector tmp;
        for (auto element : other) {
            tmp.PushBack(element);
        }
        swap(tmp);
    }

      SimpleVector( SimpleVector&& other) {
        assert(size_ == 0);
        auto it=other.begin();
        SimpleVector tmp;
        for (;it<other.end();++it) {
            tmp.PushBack(std::move(*it));
        }
        other.size_=0; 
        swap(tmp);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector rhs_copy(rhs);
            swap(rhs_copy);
        }
        return *this;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity>capacity_) {
            auto old_size=size_;
            this->Resize(new_capacity);
            size_=old_size;
        }
    }
    
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            auto old_size = size_;
            if (capacity_ == 0) {
                Resize(1);

            }
            else if (size_ == capacity_) {
                Resize(2 * size_);
            }
            storage_[old_size] = item;
            size_=++old_size;
        }
        else {
            storage_[size_] = item;
            ++size_;
        }
    }
    
    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            auto old_size = size_;
            if (capacity_ == 0) {
                Resize(1);
            }
            else if (size_ == capacity_) {
                Resize(2 * size_);
            }
            storage_[old_size] = std::move(item);
            size_=++old_size;
        }
        else {
            storage_[size_] = std::move(item);
            ++size_;
        }
    }


    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        size_t dist = std::distance(this->cbegin(),pos);
        if (size_<capacity_) {
            std::copy_backward(this->begin()+dist, this->end(),this->end()+1);
            ++size_;
            *(this->begin()+dist)=value;
            return (this->begin()+dist);
        } else if (capacity_==0) {
            this->PushBack(value);
            return this->begin();
        } else {
            ArrayPtr<Type> temp(size_*2);
            std::copy(this->begin(),this->begin()+dist,temp.Get());
            temp[dist]=value;
            std::copy(this->begin()+dist,this->end(),temp.Get()+dist+1);
            storage_.swap(temp);
            capacity_=size_*2;
            ++size_;
            return (this->begin()+dist); 
        }
    }

    Iterator Insert(ConstIterator pos,  Type&& value) {
        size_t dist = std::distance(this->cbegin(),pos);
        if (size_<capacity_) {
            std::copy_backward(std::make_move_iterator(this->begin()+dist), std::make_move_iterator(this->end()),this->end()+1);
            ++size_;
            *(this->begin()+dist)=std::move(value);
            return (this->begin()+dist);
        } else if (capacity_==0) {
            this->PushBack(std::move(value));
            return this->begin();
        } else {
            ArrayPtr<Type> temp(size_*2);
            std::copy(std::make_move_iterator(this->begin()),std::make_move_iterator(this->begin()+dist),temp.Get());
            temp[dist]=std::move(value);
            std::copy(std::make_move_iterator(this->begin()+dist),std::make_move_iterator(this->end()),temp.Get()+dist+1);
            storage_.swap(temp);
            capacity_=size_*2;
            ++size_;
            return (this->begin()+dist); 
        }
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size_ != 0) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        size_t dist = std::distance(this->cbegin(),pos);
        std::copy(std::make_move_iterator(this->begin()+dist+1), std::make_move_iterator(this->end()), this->begin()+dist);
        --size_;
        return (this->begin()+dist);
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
        storage_.swap(other.storage_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return !size_;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("index SimpleVector out of range");
        }
        return storage_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("index SimpleVector out of range");
        }
        return storage_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size < size_) {
            size_ = new_size;
        }
        else if (new_size < capacity_) {
            ArrayPtr<Type> numbers(new_size);
            Type* begin_storage_ = storage_.Get();
            Type* end_storage_ = storage_.Get();
            std::advance(end_storage_, size_);
            Type* begin_numbers = numbers.Get();
            std::copy(std::make_move_iterator(begin_storage_), std::make_move_iterator(end_storage_), begin_numbers);
            storage_.swap(numbers);
            size_ = new_size;

        }
        else if (new_size > capacity_) {
            ArrayPtr<Type> numbers(new_size);
            Type* begin_storage_ = storage_.Get();
            Type* end_storage_ = storage_.Get();
            std::advance(end_storage_, size_);
            Type* begin_numbers = numbers.Get();
            std::copy(std::make_move_iterator(begin_storage_), std::make_move_iterator(end_storage_), begin_numbers);
            storage_.swap(numbers);
            size_ = new_size;
            capacity_ = new_size;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return storage_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        auto end = storage_.Get();
        std::advance(end, size_);
        return end;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return storage_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        auto end = storage_.Get();
        std::advance(end, size_);
        return end;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return  storage_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        auto end = storage_.Get();
        std::advance(end, size_);
        return end;
    }

private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> storage_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()))
        && !(std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end()));
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
   
    return (std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end())) 
            || (std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end()));
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно
    return !(std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end()));
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно
    return (std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end()));
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    // Заглушка. Напишите тело самостоятельно
    return !(std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()));
}