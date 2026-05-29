#pragma once
#include <cmath>
#include <cassert>
#include <iostream>

template<int n> 
class Vector {
public:
    double data[n] = {0};
    double& operator[](const int i) { assert(i>=0 && i<n); return data[i]; }
    double operator[](const int i) const { assert(i>=0 && i<n); return data[i]; }

    class CommaInitializer {
        Vector& vec;
        int index = 0;
    public:
        CommaInitializer(Vector<n>& v, double& val): vec(v), index(0) {
            assert(index < n);
            vec.data[index++] = val;
        }
        CommaInitializer& operator,(const double& value) {
            assert(index < n);
            vec.data[index++] = value;
            return *this;
        }
    };
    CommaInitializer operator<<(const double& value) {
        return CommaInitializer(*this, value);
    }

    double dot(const Vector<n>& other) const {
        double result = 0;
        for (int i = 0; i < n; i++) result += data[i] * other[i];
        return result;
    }

    Vector<n> cross(const Vector<n>& other) const {
        static_assert(n == 3, "Cross product is only defined for 3D vectors.");
        Vector<n> result;
        result[0] = data[1] * other[2] - data[2] * other[1];
        result[1] = data[2] * other[0] - data[0] * other[2];
        result[2] = data[0] * other[1] - data[1] * other[0];
        return result;
    }

    double norm() const {
        return std::sqrt(this->dot(*this));
    }

    Vector<n> normalized() const {
        return *this / this->norm();
    }
};

template<int n>
Vector<n> operator+(const Vector<n>& a, const Vector<n>& b) {
    Vector<n> result;
    for (int i = 0; i < n; i++) result[i] = a[i] + b[i];
    return result;
}

template<int n> 
Vector<n> operator-(const Vector<n>& a, const Vector<n>& b) {
    Vector<n> result;
    for (int i = 0; i < n; i++) result[i] = a[i] - b[i];
    return result;
}

template<int n> 
Vector<n> operator*(const Vector<n>& v, const double& scalar) {
    Vector<n> result;
    for (int i = 0; i < n; i++) result[i] = v[i] * scalar;
    return result;
}

template<int n> 
Vector<n> operator*(const double& scalar, const Vector<n>& v) {
    return v * scalar;
}

template<int n> 
Vector<n> operator/(const Vector<n>& v, const double& scalar) {
    Vector<n> result;
    for (int i = 0; i < n; i++) result[i] = v[i] / scalar;
    return result;
}

template<int n> 
std::ostream& operator<<(std::ostream& out, const Vector<n>& v) {
    for (int i = 0; i < n; i++) {
        out << v[i];
        if (i < n - 1) out << " ";
    }
    return out;
}

// 按列有限存储的矩阵类，支持基本的矩阵运算和求逆
template<int rows, int cols> 
class Matrix {
public:
    Vector<rows> data[cols] = {};
    double& operator()(const int i, const int j) { 
        assert(i>=0 && i<rows && j>=0 && j<cols); 
        return data[j][i]; 
    }
    double operator()(const int i, const int j) const { 
        assert(i>=0 && i<rows && j>=0 && j<cols); 
        return data[j][i]; 
    }

    static Matrix<rows, cols> Identity() {
        Matrix<rows, cols> result;
        for (int i = 0; i < std::min(rows, cols); i++) result(i, i) = 1;
        return result;
    }

    static Matrix<rows, cols> Zero() {
        return Matrix<rows, cols>();
    }

    class CommaInitializer {
        Matrix& mat;
        int index = 0;
    public:
        CommaInitializer(Matrix& m, const double& val): mat(m), index(0) {
            assert(index < rows * cols);
            mat.data[index % cols][index / cols] = val;
            index++;
        }
        CommaInitializer& operator,(const double& value) {
            assert(index < rows * cols);
            mat.data[index % cols][index / cols] = value;
            index++;
            return *this;
        }
    };
    CommaInitializer operator<<(const double& value) {
        return CommaInitializer(*this, value);
    }

    template<int otherCols>
    Matrix<rows, otherCols> multiply(const Matrix<cols, otherCols>& other) const {
        Matrix<rows, otherCols> result;
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < otherCols; j++)
                for (int k = 0; k < cols; k++)
                    result(i, j) += (*this)(i, k) * other(k, j);
        return result;
    }

    Vector<rows> multiply(const Vector<cols>& vec) const {
        Vector<rows> result;
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++)
                result[i] += (*this)(i, j) * vec[j];
        return result;
    }

    Matrix<cols, rows> transpose() const {
        Matrix<cols, rows> result;
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++)
                result(j, i) = (*this)(i, j);
        return result;
    }

    double det() const {
        static_assert(rows == cols, "Determinant is only defined for square matrices.");
        if constexpr (rows == 1) {
            return (*this)(0, 0);
        } else if constexpr (rows == 2) {
            return (*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0);
        } else if constexpr (rows == 3) {
            return (*this)(0, 0) * ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1))
                 - (*this)(0, 1) * ((*this)(1, 0) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 0))
                 + (*this)(0, 2) * ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0));
        } else {
            throw std::runtime_error("Determinant calculation is only implemented for matrices up to size 3x3.");
        }
    }

    Matrix<rows, cols> inverse() const {
        static_assert(rows == cols, "Inverse is only defined for square matrices.");
        constexpr int n = rows;
        Matrix<n, 2*n> augmented;
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                augmented(i, j) = (*this)(i, j);
        for (int i = 0; i < n; i++)
            augmented(i, n + i) = 1;

        for (int i = 0; i < n; i++) {
            int pivot = i;
            for (int j = i + 1; j < n; j++)
                if (std::abs(augmented(j, i)) > std::abs(augmented(pivot, i)))
                    pivot = j;
            if (pivot != i)
                for (int k = 0; k < 2 * n; k++)
                    std::swap(augmented(i, k), augmented(pivot, k));

            double pivot_value = augmented(i, i);
            if (std::abs(pivot_value) < 1e-10)
                throw std::runtime_error("Matrix is singular and cannot be inverted.");

            for (int k = 0; k < 2 * n; k++)
                augmented(i, k) /= pivot_value;

            for (int j = 0; j < n; j++) {
                if (j == i) continue;
                double factor = augmented(j, i);
                for (int k = 0; k < 2 * n; k++)
                    augmented(j, k) -= factor * augmented(i, k);
            }
        }

        Matrix<rows, cols> inverse;
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                inverse(i, j) = augmented(i, n + j);
        return inverse;
    }
};

template<int rows, int cols>
Matrix<rows, cols> operator+(const Matrix<rows, cols>& a, const Matrix<rows, cols>& b) {
    Matrix<rows, cols> result;
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result(i, j) = a(i, j) + b(i, j);
    return result;
}

template<int rows, int cols>
Matrix<rows, cols> operator-(const Matrix<rows, cols>& a, const Matrix<rows, cols>& b) {
    Matrix<rows, cols> result;
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result(i, j) = a(i, j) - b(i, j);
    return result;
}

template<int rows, int cols>
Matrix<rows, cols> operator*(const Matrix<rows, cols>& m, const double &scalar) {
    Matrix<rows, cols> result;
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result(i, j) = m(i, j) * scalar;
    return result;
}

template<int rows, int cols>
Matrix<rows, cols> operator*(const double &scalar, const Matrix<rows, cols>& m) {
    return m * scalar;
}

template<int rows, int cols>
Matrix<rows, cols> operator/(const Matrix<rows, cols>& m, const double &scalar) {
    Matrix<rows, cols> result;
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result(i, j) = m(i, j) / scalar;
    return result;
}

template<int rows, int cols>
std::ostream& operator<<(std::ostream& os, const Matrix<rows, cols>& m) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            os << m(i, j);
            if (j < cols - 1) os << " ";
        }
        os << std::endl;
    }
    return os;
}

typedef Vector<2> Vector2;
typedef Vector<3> Vector3;
typedef Vector<4> Vector4;

typedef Matrix<3, 3> Matrix3;
typedef Matrix<4, 4> Matrix4;
