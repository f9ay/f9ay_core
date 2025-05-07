#pragma once

#include "matrix.hpp"
#include "matrix_view.hpp"
#include "tuple"

namespace f9ay {

enum class View_Type {
    origin,
    fixed,
    runtime
};

template<int Index, typename T>
[[nodiscard]] auto& getValue(T&& origin) {
    if constexpr (requires
        {
            { std::get<Index>(origin) };
        }
    ) {
        return std::get<Index>(origin);
    }
    else if constexpr (requires
    {
        { origin.template get<Index>() };
    }) {
        return origin.template get<Index>();
    }
    else {
        static_assert(false, "no implement");
    }
}

template<typename Origin_Type>
class ElementIterator_base {
public:
    Origin_Type *origin = nullptr;
    ElementIterator_base(Origin_Type *_origin): origin(_origin) {}
public:
    // ++x

    template<typename This_Type>
    This_Type operator++(this This_Type &self) {
        return ++self.origin;
    }

    // x++
    template<typename This_Type>
    This_Type operator++(this This_Type &self, int) {
        return self.origin++;
    }

    template<typename This_Type>
    This_Type operator--(this This_Type &self) {
        return --self.origin;
    }

    template<typename This_Type>
    This_Type operator--(this This_Type &self, int) {
        return self.origin--;
    }

    template<typename This_Type>
    This_Type operator+(this This_Type &self, ptrdiff_t offset) {
        return self.origin + offset;
    }

    template<typename This_Type>
    This_Type operator-(this This_Type &self, ptrdiff_t offset) {
        return self.origin - offset;
    }

    template<typename This_Type>
    auto& operator *(this This_Type &self) {
        return self.value();
    }

    bool operator != (ElementIterator_base const &other) {
        return origin != other.origin;
    }

    bool operator == (ElementIterator_base const &other) {
        return origin == other.origin;
    }
};

template<View_Type TYPE, typename Origin_Type, int Index>
class ElementIterator {
    static_assert(false, "not implemented");
};

// template<typename Origin_Type>
// class Element<View_Type::origin, Origin_Type, 0> {
// public:
//     auto& value() {
//         return *origin;
//     }
//     explicit Element(Origin_Type *_origin) : origin(_origin) {}
// private:
//     Origin_Type *origin = nullptr;
// };

template<typename Origin_Type, int Index>
class ElementIterator<View_Type::fixed, Origin_Type, Index> : public ElementIterator_base<Origin_Type> {
public:
    using ElementIterator_base<Origin_Type>::origin; // 不知道為什麼有 bug 要加這行
    auto& value() {
        return getValue<Index>(*origin);
    }
    ElementIterator(Origin_Type *_origin) : ElementIterator_base<Origin_Type>(_origin) {}
};

template<typename Origin_Type>
class ElementIterator<View_Type::runtime, Origin_Type, 0> : public ElementIterator_base<Origin_Type> {
public:
    using ElementIterator_base<Origin_Type>::origin; // 不知道為什麼有 bug 要加這行
    auto& value() {
        return (*origin)[index];
    }
    ElementIterator(Origin_Type *_origin, const int _index) : ElementIterator_base<Origin_Type>(_origin), index(_index) {}
private:
    const int index = 0;
};

// 推導指引
template<typename Origin_Type>
ElementIterator(Origin_Type &origin_type) -> ElementIterator<View_Type::origin, Origin_Type, 0>;

template<int Index, typename Origin_Type>
ElementIterator(Origin_Type &origin_type) -> ElementIterator<View_Type::fixed, Origin_Type, Index>;

template<typename Origin_Type>
ElementIterator(Origin_Type &origin_type, int _index) -> ElementIterator<View_Type::runtime, Origin_Type, 0>;

template<View_Type TYPE, typename Origin_Type, int Index>
class Row_view {
public:
    [[nodiscard]] auto begin() const {
        if constexpr (TYPE == View_Type::origin) {
            return data;
        }
        else if constexpr (TYPE == View_Type::fixed) {
            return ElementIterator<TYPE, Origin_Type, Index>(data);
        }
    }
    [[nodiscard]] auto end() const {
        if constexpr (TYPE == View_Type::origin) {
            return data;
        }
        else if constexpr (TYPE == View_Type::fixed) {
            return ElementIterator<TYPE, Origin_Type, Index>(data + cols);
        }
    }
    const auto &operator[](const int index) const {
        if constexpr (TYPE == View_Type::origin) {
            return data[index];
        }
        else if constexpr (TYPE == View_Type::fixed) {
            return getValue<Index>(data[index]);
        }
    }

    auto &operator[](const int index) {
        if constexpr (TYPE == View_Type::origin) {
            return data[index];
        }
        else if constexpr (TYPE == View_Type::fixed) {
            return getValue<Index>(data[index]);
        }
    }

    Row_view(Origin_Type * _data, const size_t _cols) : data(_data), cols(_cols) {}

    explicit Row_view(Row<Origin_Type> r) : data(r.data), cols(r.cols) {}
private:
    Origin_Type *const data = nullptr;
    const size_t cols = 0;
};

template<typename Origin_Type, int Index>
class Row_view<View_Type::runtime, Origin_Type, Index> {
public:
    [[nodiscard]] auto begin() const {
        return ElementIterator<View_Type::runtime, Origin_Type, 0>(data, index);
    }
    [[nodiscard]] auto end() const {
        return ElementIterator<View_Type::runtime, Origin_Type, 0>(data, index);
    }
    const auto &operator[](const int _index) const {
        return data[_index][index];
    }

    auto &operator[](const int _index) {
        return data[_index][index];
    }

    Row_view(Origin_Type * _data, const size_t _cols, const int _index) : data(_data), cols(_cols), index(_index) {}

    Row_view(Row<Origin_Type> r, const int _index) : data(r.data), cols(r.cols), index(_index) {}
private:
    Origin_Type *const data = nullptr;
    const size_t cols = 0;
    const int index = 0;
};

// 推導指南
template<typename Origin_Type>
Row_view(Origin_Type *data, size_t cols) -> Row_view<View_Type::origin, Origin_Type, 0>;

template<int Index, typename Origin_Type>
Row_view(Origin_Type *data, size_t cols) -> Row_view<View_Type::fixed, Origin_Type, Index>;

template<View_Type TYPE>
struct Optional_Index {};

template<>
struct Optional_Index<View_Type::runtime> {
    int index;
    explicit Optional_Index(const int _index) : index(_index) {}
};


template<View_Type TYPE, typename Origin_Type, int Index>
class RowIterator_view : protected Optional_Index<TYPE> {
private:
    Origin_Type *data;
    size_t size;
public:

    RowIterator_view(Origin_Type *_data, const size_t _size) requires requires {TYPE != View_Type::runtime;} : data(_data), size(_size) {}

    RowIterator_view(Origin_Type *_data, const size_t _size, const int _index) requires requires {TYPE == View_Type::runtime;} : Optional_Index<TYPE>(_index), data(_data), size(_size) {}

    explicit RowIterator_view(RowIterator<Origin_Type> it) requires requires {TYPE != View_Type::runtime;} : data(it.data), size(it.size) {}

    RowIterator_view(RowIterator<Origin_Type> it, const int _index) requires requires {TYPE == View_Type::runtime;} : Optional_Index<TYPE>(_index), data(it.data), size(it.size) {}

    // ++x
    RowIterator_view operator++() {
        data += size;
        return *this;
    }

    // x++
    RowIterator_view operator++(int) {
        auto temp = *this;
        data += size;
        return temp;
    }

    RowIterator_view operator--() {
        auto temp = *this;
        data -= size;
        return temp;
    }

    RowIterator_view operator--(int) {
        auto temp = *this;
        data -= size;
        return temp;
    }

    RowIterator_view operator+(ptrdiff_t offset) const {
        return RowIterator(data + offset, size);
    }

    RowIterator_view operator-(ptrdiff_t offset) const {
        return RowIterator(data - offset, size);
    }

    auto operator==(const RowIterator_view &other) const {
#ifdef DEBUG
        if (size != other.size) {
            throw std::runtime_error("RowIterator sizes do not match");
        }
#endif
        return data == other.data;
    }

    auto operator!=(const RowIterator_view &other) const {
#ifdef DEBUG
        if (size != other.size) {
            throw std::runtime_error("RowIterator sizes do not match");
        }
#endif
        return data != other.data;
    }

    Row_view<TYPE, Origin_Type, Index> operator*() const {
        if constexpr (TYPE == View_Type::runtime) {
            return Row_view<TYPE, Origin_Type, Index>(data, size, Optional_Index<TYPE>::index);
        }
        return Row_view<TYPE, Origin_Type, Index>(data, size);
    }

    Row_view<TYPE, Origin_Type, Index> operator*() {
        if constexpr (TYPE == View_Type::runtime) {
            return Row_view<TYPE, Origin_Type, Index>(data, size, Optional_Index<TYPE>::index);
        }
        return Row_view<TYPE, Origin_Type, Index>(data, size);
    }
};


template<View_Type TYPE, typename Origin_Type, int Index>
class Matrix_view {
    static_assert(false, "no implementation for Matrix_View");
};

template<typename Origin_Type>
class Matrix_view<View_Type::origin, Origin_Type, 0> {
public:
    auto begin() {
        return (*origin).begin();
    }
    auto end() {
        return (*origin).begin();
    }

    const auto &operator[](size_t i, size_t j) const {
        return (*origin)[i, j];
    }

    auto &operator[](size_t i, size_t j) {
        return (*origin)[i, j];
    }

    auto operator[](size_t i) const {
        return (*origin)[i];
    }

    auto operator[](size_t i) {
        return (*origin)[i];
    }

    [[nodiscard]] int row() const {
        return origin->row();
    }

    [[nodiscard]] int col() const {
        return origin->col();
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    Matrix_view(Matrix<Origin_Type> &_origin) : origin(&_origin) {}
private:
    Matrix<Origin_Type> *origin = nullptr;
};

template<typename Origin_Type, int Index>
class Matrix_view<View_Type::fixed, Origin_Type, Index> {
    static constexpr auto TYPE = View_Type::fixed;
public:
    auto begin() {
        return RowIterator_view<TYPE, Origin_Type, Index>(origin->begin());
    }
    auto end() {
        return RowIterator_view<TYPE, Origin_Type, Index>(origin->end());
    }

    const auto &operator[](size_t i, size_t j) const {
        return getValue<Index>((*origin)[i, j]);
    }

    auto &operator[](size_t i, size_t j) {
        return getValue<Index>((*origin)[i, j]);
    }

    auto operator[](size_t i) const {
        return Row_view<TYPE, Origin_Type, Index>((*origin)[i]);
    }

    auto operator[](size_t i) {
        return Row_view<TYPE, Origin_Type, Index>((*origin)[i]);
    }

    [[nodiscard]] int row() const {
        return origin->row();
    }

    [[nodiscard]] int col() const {
        return origin->col();
    }
    Matrix_view(Matrix<Origin_Type> &_origin) : origin(&_origin) {}
private:
    Matrix<Origin_Type> *origin = nullptr;
};

template<typename Origin_Type>
class Matrix_view<View_Type::runtime, Origin_Type, 0> {
    static constexpr auto TYPE = View_Type::fixed;
public:
    auto begin() {
        return RowIterator_view<TYPE, Origin_Type, 0>(origin->begin(), index);
    }
    auto end() {
        return RowIterator_view<TYPE, Origin_Type, 0>(origin->end(), index);
    }

    const auto &operator[](size_t i, size_t j) const {
        return ((*origin)[i, j])[index];
    }

    auto &operator[](size_t i, size_t j) {
        return ((*origin)[i, j])[index];
    }

    auto operator[](size_t i) const {
        return Row_view<TYPE, Origin_Type, 0>((*origin)[i], index);
    }

    auto operator[](size_t i) {
        return Row_view<TYPE, Origin_Type, 0>((*origin)[i], index);
    }

    [[nodiscard]] int row() const {
        return origin->row();
    }

    [[nodiscard]] int col() const {
        return origin->col();
    }
    Matrix_view(Matrix<Origin_Type> &_origin, const int _index) : origin(&_origin), index(_index) {}
private:
    Matrix<Origin_Type> *origin = nullptr;
    const int index;
};

template<int Index, typename Origin_Type>
Matrix_view<View_Type::fixed, Origin_Type, Index> Matrix_view_fixed(Matrix<Origin_Type> &_origin) {
    return Matrix_view<View_Type::fixed, Origin_Type, Index>(_origin);
}

// 推導指引
template<typename Origin_Type>
Matrix_view(Matrix<Origin_Type> &_origin) -> Matrix_view<View_Type::origin, Origin_Type, 0>;

template<int Index, typename Origin_Type>
Matrix_view(Matrix<Origin_Type> &_origin) -> Matrix_view<View_Type::fixed, Origin_Type, Index>;


template<typename Origin_Type>
Matrix_view(Matrix<Origin_Type> &_origin, int _index) -> Matrix_view<View_Type::runtime, Origin_Type, 0>;
//                                              ^ 他不讓我加 const



};

template<f9ay::View_Type TYPE, typename Origin_Type, int Index, typename Char_T>
struct std::formatter<f9ay::Matrix_view<TYPE, Origin_Type, Index>, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::Matrix_view<TYPE, Origin_Type, Index> &matrix, auto &ctx) const {
        auto out = ctx.out();
        for (int i = 0; i < matrix.row(); i++) {
            for (int j = 0; j < matrix.col(); j++) {
                out = std::format_to(out, "{} ", matrix[i, j]);
            }
            out = std::format_to(out, "\n");
        }
        return out;
    }
};
