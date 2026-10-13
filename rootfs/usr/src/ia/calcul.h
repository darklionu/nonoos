#pragma once

#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace calcul {

template <typename T>
inline auto addition(const T& a, const T& b) noexcept -> decltype(a + b) {
    return a + b;
}

template <typename T>
inline auto soustraction(const T& a, const T& b) noexcept -> decltype(a - b) {
    return a - b;
}

template <typename T>
inline auto multiplication(const T& a, const T& b) noexcept -> decltype(a * b) {
    return a * b;
}

template <typename T>
inline auto division(const T& a, const T& b) -> decltype(a / b) {
    if (b == static_cast<T>(0)) {
        throw std::domain_error("Division par zero");
    }
    return a / b;
}

template <typename T>
inline auto modulo(const T& a, const T& b) -> decltype(a % b) {
    static_assert(std::is_integral<T>::value, "modulo est réservé aux types entiers");
    if (b == static_cast<T>(0)) {
        throw std::domain_error("Modulo par zero");
    }
    return a % b;
}

template <typename T>
inline auto carre(const T& value) noexcept -> decltype(value * value) {
    return value * value;
}

template <typename T>
inline auto cube(const T& value) noexcept -> decltype(value * value * value) {
    return value * value * value;
}

template <typename T>
inline auto puissance(const T& base, int exponent) noexcept -> T {
    if (exponent == 0) {
        return static_cast<T>(1);
    }
    T result = static_cast<T>(1);
    if (exponent < 0) {
        T absBase = base == static_cast<T>(0) ? static_cast<T>(0) : static_cast<T>(1) / base;
        int absExponent = -exponent;
        for (int i = 0; i < absExponent; ++i) {
            result *= absBase;
        }
        return result;
    }
    for (int i = 0; i < exponent; ++i) {
        result *= base;
    }
    return result;
}

template <typename T>
inline auto racineCarree(const T& value) -> double {
    if (value < static_cast<T>(0)) {
        throw std::domain_error("Racine carree d'un nombre negatif");
    }
    return std::sqrt(static_cast<double>(value));
}

template <typename T>
inline auto minimum(const T& a, const T& b) noexcept -> T {
    return a < b ? a : b;
}

template <typename T>
inline auto maximum(const T& a, const T& b) noexcept -> T {
    return a > b ? a : b;
}

template <typename T>
inline auto factorielle(const T& value) -> long long {
    if (value < static_cast<T>(0)) {
        throw std::domain_error("Factorielle d'un nombre negatif");
    }
    long long result = 1;
    for (T i = static_cast<T>(1); i <= value; ++i) {
        result *= static_cast<long long>(i);
    }
    return result;
}

template <typename T>
inline auto estPremier(const T& value) noexcept -> bool {
    if (value <= static_cast<T>(1)) {
        return false;
    }
    if (value == static_cast<T>(2)) {
        return true;
    }
    if ((value % static_cast<T>(2)) == static_cast<T>(0)) {
        return false;
    }

    for (T i = static_cast<T>(3); i * i <= value; i += static_cast<T>(2)) {
        if ((value % i) == static_cast<T>(0)) {
            return false;
        }
    }
    return true;
}

template <typename T>
inline auto pgcd(const T& a, const T& b) noexcept -> T {
    T x = a < 0 ? -a : a;
    T y = b < 0 ? -b : b;
    while (y != static_cast<T>(0)) {
        T tmp = x % y;
        x = y;
        y = tmp;
    }
    return x;
}

template <typename T>
inline auto ppcm(const T& a, const T& b) noexcept -> T {
    if (a == static_cast<T>(0) || b == static_cast<T>(0)) {
        return static_cast<T>(0);
    }
    return (a / pgcd(a, b)) * b;
}

// Aliases français 
template <typename T>
inline auto somme(const T& a, const T& b) noexcept -> decltype(a + b) {
    return addition(a, b);
}

template <typename T>
inline auto difference(const T& a, const T& b) noexcept -> decltype(a - b) {
    return soustraction(a, b);
}

template <typename T>
inline auto produit(const T& a, const T& b) noexcept -> decltype(a * b) {
    return multiplication(a, b);
}

template <typename T>
inline auto quotient(const T& a, const T& b) -> decltype(a / b) {
    return division(a, b);
}

template <typename T>
inline auto reste(const T& a, const T& b) -> decltype(a % b) {
    return modulo(a, b);
}

template <typename T>
inline auto moyenne(const T& a, const T& b) -> decltype((a + b) / static_cast<T>(2)) {
    return (a + b) / static_cast<T>(2);
}

}  // namespace calcul

// Version globale pour les usages les plus simples.
template <typename T>
inline auto add(const T& a, const T& b) noexcept -> decltype(a + b) {
    return calcul::addition(a, b);
}

template <typename T>
inline auto sub(const T& a, const T& b) noexcept -> decltype(a - b) {
    return calcul::soustraction(a, b);
}

template <typename T>
inline auto mul(const T& a, const T& b) noexcept -> decltype(a * b) {
    return calcul::multiplication(a, b);
}

template <typename T>
inline auto div(const T& a, const T& b) -> decltype(a / b) {
    return calcul::division(a, b);
}

template <typename T>
inline auto mod(const T& a, const T& b) -> decltype(a % b) {
    return calcul::modulo(a, b);
}

