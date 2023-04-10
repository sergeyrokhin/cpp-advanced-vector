#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
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
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(other.data_.GetAddress(), size_, data_.GetAddress());
        }
        else {
            std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
        }
    }

    Vector(Vector&& other) noexcept {
        Swap(other);
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

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                if (size_ < rhs.size_)
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                else
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);

                auto copy_count = std::min(size_, rhs.size_);
                size_ = rhs.size_;
                for (size_t i = 0; i < copy_count; i++)
                {
                    operator[](i) = rhs[i];
                }
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            size_ = 0;
            Swap(rhs);
        }
        return *this;
    }

    void Swap(Vector& other) noexcept {
        std::swap(size_, other.size_);
        data_.Swap(other.data_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (size_ < new_size)
        {
            if (data_.Capacity() < new_size) {
                Reserve(new_size);
            }
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        else { //новый размер меньше, уничтожим лишнее
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        size_ = new_size;
    }

    void PushBack(const T& value) {
        if (size_ == Capacity()) {

            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);

            new (new_data + size_) T(value);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else
            new (data_ + size_) T(value);
        ++size_;
    }

    void PushBack(T&& value) {
        if (size_ == Capacity()) {

            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);

            new (new_data + size_) T(std::move(value));

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else
            new (data_ + size_) T(std::move(value));
        ++size_;
    }

    void PopBack() noexcept { /* noexcept */
        if (size_ == 0)
        {
            return;
        }
        --size_;
        std::destroy_n(data_.GetAddress() + size_, 1);
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {

            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);

            new (new_data.GetAddress() + size_) T(std::forward<Args>(args) ...);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else
            new (data_ + size_) T(std::forward<Args>(args) ...);
        ++size_;
        return data_[size_ - 1];
    }

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {

        auto cursor = std::distance(begin(), const_cast<iterator>(pos));
        if (std::distance(pos,cend()) == 0)
        {
            EmplaceBack(std::forward<Args>(args) ...);
        }
        else {
            if (size_ == Capacity()) {
                RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);

                new (new_data.GetAddress() + cursor) T(std::forward<Args>(args) ...);


                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::uninitialized_move(begin(), const_cast<iterator>(pos), new_data.GetAddress());
                    std::uninitialized_move(const_cast<iterator>(pos), end(), new_data.GetAddress() + cursor + 1);
                }
                else {
                    std::uninitialized_copy(begin(), const_cast<iterator>(pos), new_data.GetAddress());
                    std::uninitialized_copy(const_cast<iterator>(pos), end(), new_data.GetAddress() + cursor + 1);
                }
                data_.Swap(new_data);
                std::destroy_n(new_data.GetAddress(), size_);
            }
            else {
                RawMemory<T> new_value(1);
                new (new_value.GetAddress()) T(std::forward<Args>(args) ...);
                if constexpr (std::is_nothrow_move_assignable_v<T> || !std::is_copy_assignable_v<T>) {
                    std::uninitialized_move_n(end() - 1, 1, end());
                    std::move_backward(const_cast<iterator>(pos), end() - 1, end());
                    *(data_.GetAddress() + cursor) = std::move(*(new_value.GetAddress()));
                }
                else {
                    std::uninitialized_copy_n(end() - 1, 1, end());
                    std::copy_backward(const_cast<iterator>(pos), end() - 1, end());
                    *(data_.GetAddress() + cursor) = *(new_value.GetAddress());
                }
                std::destroy_n(new_value.GetAddress(), 1);
            }
            ++size_;

        }
        return data_.GetAddress() + cursor;
    }
    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        if constexpr (std::is_nothrow_move_assignable_v<T>)
            std::move(const_cast<iterator>(pos) + 1, end(), const_cast<iterator>(pos));
        else
            std::copy(const_cast<iterator>(pos) + 1, end(), const_cast<iterator>(pos));
        --size_;
        std::destroy_n(data_.GetAddress() + size_, 1);
        return const_cast<iterator>(pos);
    }
    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};