#pragma once
#include <compare>
#include <cstddef>
#include <format>
#include <iostream>
#include <span>

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

    Matrix(const int _rows, const int _cols) : rows(_rows), cols(_cols) {
        data = new T[rows * cols]{};
    }
    Matrix(const Matrix &mtx) : rows(mtx.rows), cols(mtx.cols) {
        data = new T[rows * cols]{};
        for (size_t i = 0; i < rows * cols; i++) {
            data[i] = mtx.data[i];
        }
    }
    Matrix &operator=(const Matrix &) = delete;
    Matrix(Matrix &&other) noexcept
        : data(other.data), rows(other.rows), cols(other.cols) {
        other.data = nullptr;
        other.rows = 0;
        other.cols = 0;
    }

    Matrix inverse() const {
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
    ~Matrix() {
        delete[] data;
    }

private:
    T *data = nullptr;
    int rows, cols;
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
struct std::formatter<f9ay::Matrix<T>, Char_T>
    : std::formatter<std::string, Char_T> {
    auto format(const f9ay::Matrix<T> &matrix, auto &ctx) const {
        auto out = ctx.out();
        for (int i = 0; i < matrix.row(); i++) {
            for (int j = 0; j < matrix.col(); j++) {
                out = std::format_to(out, "{} ", matrix[i][j]);
            }
            out = std::format_to(out, "\n");
        }
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