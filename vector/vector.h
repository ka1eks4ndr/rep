#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <iterator>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {  
        capacity_ = other.capacity_;
        buffer_ = other.buffer_;
        other.buffer_ = nullptr;
        //other.~RawMemory();
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept { 
        this->capacity_ = rhs.capacity_;
        this->buffer_ = rhs.buffer_;
        rhs.buffer_ = nullptr; 
        //rhs.~RawMemory();
        return *this;
        }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return (data_+size_);
    }
    
    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return (data_+size_);
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return (data_+size_);
    }

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
        
    } 

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
        
    }

    Vector(Vector&& other) noexcept {
        data_ = std::move(other.data_);
        size_ = std::move(other.size_);
        other.size_ = 0;
        
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                auto temp(rhs);
                Swap(temp);
            } else {
                if (size_>rhs.Size()) {
                    for (size_t i =0; i<rhs.Size() ; ++i) {
                        data_[i] = rhs[i];
                    }
                    std::destroy_n(data_+rhs.size_-1, size_-rhs.size_);
                } else {
                    size_t i =0;
                    for (; i<size_ ; ++i) {
                        data_[i] = rhs[i];
                    }
                    std::uninitialized_copy_n(rhs.data_+i,rhs.size_-i,data_+i);
                }
                
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        data_ = std::move(rhs.data_);
        size_ = std::move(rhs.size_);
        rhs.size_ = 0;
        return *this;
    }

    void Swap(Vector& other) noexcept {
        std::swap (data_, other.data_);
        std::swap (size_, other.size_);
    } 

    

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data (new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    
    void Resize(size_t new_size) {
        if (size_>new_size) {
            std::destroy_n(data_+new_size, size_-new_size);
        } else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_+size_, new_size-size_);
        }
        size_ = new_size;
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == data_.Capacity() && size_ != 0) {
            RawMemory<T> new_data(data_.Capacity()*2);
            new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else if (data_.Capacity() ==0 ) {
            Reserve(1);
            new (data_.GetAddress()) T(std::forward<Args>(args)...);
        } else {
            new (data_ + size_) T(std::forward<Args>(args)...);
            
        }
        ++size_;
        return data_[size_-1];
    }
    
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        iterator pos_result = const_cast<iterator>(pos);
        if (size_ == data_.Capacity() && size_ != 0) {
            RawMemory<T> new_data(data_.Capacity()*2);
            //создание элемента в требуемоей позиции в новом векторе.
            auto new_pos = new (new_data + (pos-this->begin())) T(std::forward<Args>(args)...);
            //копирование элементов с старого вектора до вставляемого элемента
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), (pos-this->begin()), new_data.GetAddress());
                std::uninitialized_move_n(pos_result, (this->end()-pos), new_pos+1);
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), (pos-this->begin()), new_data.GetAddress());
                std::uninitialized_copy_n(pos_result, (this->end()-pos), new_pos+1);
            }
            
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            pos_result = new_pos;
        } else if (data_.Capacity() ==0 ) {
            int distance = std::distance(pos_result,this->begin());
            Reserve(1);
            pos_result = new (data_+distance) T(std::forward<Args>(args)...);
        } else  {
            if (pos_result == cend()) {
                pos_result = new (end()) T(std::forward<Args>(args)...);
            } else {
                auto dist = pos_result-begin();
                RawMemory<T> new_data(1);
                auto temp = new (new_data.GetAddress()) T(std::forward<Args>(args)...);
                new (data_ + size_) T(std::move(*(data_.GetAddress() + size_ - 1u)));
                std::move_backward(pos_result,this->end()-1,this->end());
                pos_result = begin()+dist;
                *pos_result = std::move(*temp);
                std::destroy_at(new_data.GetAddress());
            }
        }
        ++size_;
        return pos_result;
    }
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
        if (pos != end()-1) {
            auto dist = pos - begin();
            std::move(begin()+dist+1,end(),begin()+dist);
            PopBack();
            return begin()+dist;

        }
        PopBack();
        return end();

    }
    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos,value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos,std::move(value));
    }

    void PopBack() /* noexcept */ {
        std::destroy_at(data_+size_-1);
        --size_;
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
        
    }

private:

    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    // Создаёт копию объекта elem в сырой памяти по адресу buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    // Вызывает деструктор объекта по адресу buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }

    RawMemory<T> data_;
    size_t size_ = 0;
};