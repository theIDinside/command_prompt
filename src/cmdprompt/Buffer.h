//
// Created by cx on 2018-11-22.
//

#pragma once
#include <algorithm>
#include <memory>
#include <string_view>
#include <sstream>

template <std::size_t N=80>
class Buffer {
public:
    using view = std::string_view;
    // public member functions
    Buffer(std::size_t init_capacity=N) : capacity(init_capacity+1), size(0) {
        auto deleter = [](auto item) {
            delete item;
        };
        m_data = std::unique_ptr<char[], decltype(deleter)>(new char[capacity], deleter);
        std::fill_n(*m_data, *m_data+capacity, '\0');
    }
    ~Buffer() {}
    void push_back(char c);
    char pop_back();
    void clear();
    view sub_str(std::size_t begin=0, std::size_t len=0);
    constexpr view to_view();
    bool is_empty();
private:
    // private methods

public:
    // public data members

private:
    // private data members
    std::unique_ptr<char[]> m_data;
    std::size_t size;
    std::size_t capacity;
};

template<size_t N>
void Buffer<N>::push_back(char c) {
    if(size < capacity-1) {
        *m_data[size] = c;
        ++size;
    } else if(size == capacity-1) {
        char temp_append[capacity*2];
        std::fill_n(temp_append, temp_append+(capacity*2), '\0');
        std::copy(*m_data, *m_data+capacity, temp_append);
        auto deleter = [](auto i) { delete i; };
        m_data = std::make_unique<char[], decltype(deleter)>(std::move(temp_append), deleter);
        capacity *= 2;
        *m_data[size] = c;
        ++size;
    }
}

template<size_t N>
char Buffer<N>::pop_back() {
    if(size == 0)
        throw std::range_error{"Can't remove data from buffer, size is 0"};
    char c = *m_data[size];
    *m_data[size] = '\0';
    --size;
    return c;
}

template<size_t N>
void Buffer<N>::clear() {
    std::fill_n(*m_data, *m_data+capacity,'\0');
    size = 0;
}

template<size_t N>
bool Buffer<N>::is_empty() {
    return size == 0;
}

template<size_t N>
std::string_view Buffer<N>::sub_str(std::size_t begin, std::size_t len) {
    len = (len == 0) ? (size-begin) : (len);
    if(begin + len > size)
        throw std::out_of_range{to_string("Substring from ", begin, " to ", (begin+len), " is out of range, ", size)};
    if(begin != 0 && len == size) len = size-begin;
    return view{*m_data+begin, len};
}

template<size_t N>
constexpr Buffer<>::view Buffer<N>::to_view() {
    return std::string_view{*m_data, size};
}

template <typename ...Args>
std::string to_string(Args&&... args) {
    std::stringstream ss{};
    ss << (args + ...) << std::endl;
    return ss.str();
}

template <typename ...Args>
std::string format(const std::string& format_str, Args&&... args) {

}

