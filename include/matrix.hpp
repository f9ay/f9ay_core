#pragma once
#include <cmath>
#include <compare>
#include <cstddef>
#include <format>
#include <iostream>
#include <print>
#include <span>
#include <utility>

namespace f9ay {
template <typename T>
class Row {
public:
    [[nodiscard]] auto begin() const {
        return data;
    }
    [[nodiscard]] auto end() const {
        return data + cols;
    }
    const T &operator[](size_t j) const {
#ifdef DEBUG
        if (j >= cols) {
            throw std::out_of_range("Index out of range");
        }
#endif
        return data[j];
    }
    T &operator[](size_t j) {
#ifdef DEBUG
        if (j >= cols) {
            throw std::out_of_range("Index out of range");
        }
#endif
        return data[j];
    }
    Row(T *const data, const size_t cols) : data(data), cols(cols) {}
    Row(const Row &other) : data(other.data), cols(other.cols) {}

    // private:
    T *const data = nullptr;
    const size_t cols = 0;
};

template <typename T>
class RowIterator {
public:
    T *data;
    size_t size;

public:
    RowIterator(T *_data, const size_t _size) : data(_data), size(_size) {}

    // ++x
    RowIterator operator++() {
        data += size;
        return *this;
    }

    // x++
    RowIterator operator++(int) {
        auto temp = *this;
        data += size;
        return temp;
    }

    RowIterator operator--() {
        auto temp = *this;
        data -= size;
        return temp;
    }

    RowIterator operator--(int) {
        auto temp = *this;
        data -= size;
        return temp;
    }

    RowIterator operator+(ptrdiff_t offset) const {
        return RowIterator(data + offset, size);
    }

    RowIterator operator-(ptrdiff_t offset) const {
        return RowIterator(data - offset, size);
    }

    auto operator==(const RowIterator &other) const {
#ifdef DEBUG
        if (size != other.size) {
            throw std::runtime_error("RowIterator sizes do not match");
        }
#endif
        return data == other.data;
    }

    auto operator!=(const RowIterator &other) const {
#ifdef DEBUG
        if (size != other.size) {
            throw std::runtime_error("RowIterator sizes do not match");
        }
#endif
        return data != other.data;
    }

    Row<T> operator*() const {
        return Row<T>(data, size);
    }

    Row<T> operator*() {
        return Row<T>(data, size);
    }
};

template <typename T>
class Matrix {
public:
    RowIterator<T> begin() {
        return RowIterator(data, cols);
    }
    RowIterator<T> end() {
        return RowIterator(data + (rows * cols), cols);
    }

    const T &operator[](size_t i, size_t j) const {
#ifdef DEBUG
        if (i >= rows || j >= cols) {
            throw std::out_of_range("Index out of range");
        }
#endif
        return data[i * cols + j];
    }

    T &operator[](size_t i, size_t j) {
#ifdef DEBUG
        if (i >= rows || j >= cols) {
            throw std::out_of_range("Index out of range");
        }
#endif
        return data[i * cols + j];
    }

    Row<T> operator[](size_t i) const {
#ifdef DEBUG
        if (i >= rows) {
            throw std::out_of_range("Index out of range");
        }
#endif

        return Row<T>(data + (i * cols), cols);
    }

    Row<T> operator[](size_t i) {
#ifdef DEBUG
        if (i >= rows) {
            throw std::out_of_range("Index out of range");
        }
#endif
        return Row<T>(data + (i * cols), cols);
    }

    [[nodiscard]] int row() const {
        return rows;
    }

    [[nodiscard]] int col() const {
        return cols;
    }

    [[nodiscard]] T *raw() {
        return data;
    }

    [[nodiscard]] const T *raw() const {
        return data;
    }

    [[nodiscard]] T *transfer_ownership() {
        auto temp = data;
        data = nullptr;
        cols = 0;
        rows = 0;
        return temp;
    }

    // TODO 用表達式模板優化
    template <typename Func>
    auto &transform(Func &&func) {
        for (int i = 0; i < row(); i++) {
            for (int j = 0; j < col(); j++) {
                func((*this)[i, j]);
            }
        }
        return *this;
    }

    template <typename Func>
    auto &for_each(Func &&func) const {
        for (int i = 0; i < row(); i++) {
            for (int j = 0; j < col(); j++) {
                func((*this)[i, j]);
            }
        }
        return *this;
    }

    template <typename Func>
    auto &for_each(Func &&func) {
        for (int i = 0; i < row(); i++) {
            for (int j = 0; j < col(); j++) {
                func((*this)[i, j]);
            }
        }
        return *this;
    }

    template <typename Func>
    auto trans_convert(Func &&func) const {
        Matrix<std::decay_t<decltype(func((*this)[0, 0]))>> result(row(), col());
        for (int i = 0; i < row(); i++) {
            for (int j = 0; j < col(); j++) {
                result[i, j] = func((*this)[i, j]);
            }
        }
        return result;
    }

    template <typename U>
    decltype(auto) round_div(const U &other) {
#pragma loop(hint_parallel(0))
        for (int i = 0; i < row(); i++) {
            for (int j = 0; j < col(); j++) {
                if constexpr (std::is_integral_v<T>) {
                    (*this)[i, j] = std::round((*this)[i, j] / float(other[i][j]));
                } else {  // 浮點 或 其他不管
                    (*this)[i, j] /= other[i][j];
                }
            }
        }
        return *this;
    }

    template <typename U>
    auto round_div_convert(const U &other) const {
        Matrix<T> result(row(), col());
        for (int i = 0; i < row(); i++) {
            for (int j = 0; j < col(); j++) {
                if constexpr (std::is_integral_v<T>) {
                    result[i, j] = std::round((*this)[i, j] / float(other[i][j]));
                } else {  // 浮點 或 其他不管
                    result[i, j] = (*this)[i, j] / other[i][j];
                }
            }
        }
        return result;
    }

    decltype(auto) dump() {
        println("{}", *this);
        return *this;
    }

    decltype(auto) dump_abort() {
        std::println("{}", *this);
        abort();
        return *this;
    }

    // lateinit
    Matrix() : data(nullptr), rows(0), cols(0) {}
    Matrix(const int _rows, const int _cols) : rows(_rows), cols(_cols) {
        data = new T[rows * cols]{};
    }
    Matrix(const Matrix &mtx) : rows(mtx.rows), cols(mtx.cols) {
        data = new T[rows * cols]{};
        for (size_t i = 0; i < rows * cols; i++) {
            data[i] = mtx.data[i];
        }
    }
    Matrix &operator=(Matrix &&other) {
        if (this == &other) {
            return *this;
        }
        if (data != nullptr) {
            std::println(
                "warnning matrix 的移動 operator= 賦值給不是 lateinit 的 "
                "matrix");
            delete[] data;
        }
        data = other.data;
        rows = other.rows;
        cols = other.cols;
        other.data = nullptr;
        other.rows = 0;
        other.cols = 0;
        return *this;
    }
    Matrix &operator=(const Matrix &other) {
        if (this == &other) {
            return *this;
        }
        if (data != nullptr) {
            std::println(
                "warnning matrix 的複製 operator= 賦值給不是 lateinit 的 "
                "matrix");
            delete[] data;
        }
        rows = other.rows;
        cols = other.cols;
        std::copy(other.data, other.data + other.rows * other.cols, data);
        return *this;
    }
    Matrix(Matrix &&other) noexcept : data(other.data), rows(other.rows), cols(other.cols) {
        if (this == &other) {
            return;
        }
        other.data = nullptr;
        other.rows = 0;
        other.cols = 0;
    }

    [[nodiscard]] Matrix transpose() const {
        Matrix result(cols, rows);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[j, i] = data[i * cols + j];
            }
        }
        return result;
    }
    std::span<T> flattenToSpan() {
        return std::span<T>(data, rows * cols);
    }

    void swap(Matrix &other) noexcept {
        std::swap(data, other.data);
        std::swap(rows, other.rows);
        std::swap(cols, other.cols);
    }
    /*
        {{1, 2, 3},
         {4, 5, 6},
         {7, 8, 9}}
    */
    Matrix(std::initializer_list<std::initializer_list<T>> &&list) {
        rows = list.size();
        for (auto &row : list) {
            cols = std::max(cols, static_cast<int>(row.size()));
        }
        data = new T[rows * cols]{};
        int i = 0;
        for (auto &row : list) {
            std::copy(row.begin(), row.end(), data + (i * cols));
            i++;
        }
    }

    Matrix(const std::span<T> &span, const int _rows, const int _cols) : rows(_rows), cols(_cols) {
        data = new T[span.size()];
        std::copy(span.begin(), span.end(), data);
    }

    Matrix(std::vector<T> &&vec, const int _rows, const int _cols) : rows(_rows), cols(_cols) {
        data = the_pointer_heist(vec);
    }

    Matrix(const T *data, const int _rows, const int _cols) : rows(_rows), cols(_cols) {
        this->data = new T[rows * cols];
        std::copy(data, data + (rows * cols), this->data);
    }

    ~Matrix() {
        delete[] data;
    }

private:
    T *data = nullptr;
    int rows, cols;

    decltype(auto) self() {
        return *this;
    }
};

inline std::ostream &operator<<(std::ostream &os, const Matrix<int> &matrix) {
    for (int i = 0; i < matrix.row(); i++) {
        for (int j = 0; j < matrix.col(); j++) {
            os << matrix[i][j] << " ";
        }
        os << "\n";
    }
    return os;
}

};  // namespace f9ay

template <typename T, typename Char_T>
struct std::formatter<f9ay::Matrix<T>, Char_T> : std::formatter<std::string, Char_T> {
    auto format(const f9ay::Matrix<T> &matrix, auto &ctx) const {
        auto out = ctx.out();
        out = std::format_to(
            out,
            "================================================="
            "===============================\n");
        for (int i = 0; i < matrix.row(); i++) {
            for (int j = 0; j < matrix.col(); j++) {
                if constexpr (std::is_same_v<T, std::byte>) {
                    out = std::format_to(out, "{:02x} ", static_cast<unsigned int>(matrix[i][j]));
                } else {
                    out = std::format_to(out, "{} ", matrix[i][j]);
                }
            }
            out = std::format_to(out, "\n");
        }
        out = std::format_to(
            out,
            "================================================="
            "===============================\n");
        return out;
    }
};

/*
    Matrix mtx;

    mtx.begin() ==> RowIterator as b
    mtx.end() ==> RowIterator as e
    b + rows = e

    (*RowIterator).begin ==> T*
    (*RowIterator).end ==> T*



*/
/**/