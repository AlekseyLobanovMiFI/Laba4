#ifndef ORDINAL_H
#define ORDINAL_H

#include <stdexcept>
#include <string>
#include <iostream>

// ============================================================
//  Ordinal — ординальные числа вида ω·k + n
//
//  Используется как тип индекса в LazySequence::Get.
//  Позволяет обращаться к элементам склеенных бесконечных
//  последовательностей через трансфинитные индексы:
//
//    Ordinal(5)         = 5          — обычный конечный индекс
//    Ordinal(1, 0)      = ω          — первый элемент второй бесконечной
//    Ordinal(1, 10)     = ω + 10     — одиннадцатый элемент второй бесконечной
//    Ordinal(2, 10)     = ω·2 + 10   — одиннадцатый элемент третьей бесконечной
//
//  k — номер «блока» (коэффициент при ω, нумерация с 0)
//  n — смещение внутри блока
// ============================================================

class Ordinal {
public:
    int k;  // коэффициент при ω (≥ 0)
    int n;  // конечная часть (≥ 0)

    // Конечный ординал: просто число n (k = 0)
    Ordinal(int n) : k(0), n(n) {
        if (n < 0) throw std::out_of_range("Ordinal: negative value");
    }

    // Трансфинитный ординал: ω·k + n
    Ordinal(int k, int n) : k(k), n(n) {
        if (k < 0 || n < 0)
            throw std::out_of_range("Ordinal: negative value");
    }

    bool IsFinite()   const { return k == 0; }
    bool IsInfinite() const { return k > 0; }

    // ---- Фабрики ----
    static Ordinal Finite(int n)           { return Ordinal(0, n); }
    static Ordinal Omega()                 { return Ordinal(1, 0); }
    static Ordinal OmegaK(int k)          { return Ordinal(k, 0); }
    static Ordinal OmegaKPlusN(int k, int n) { return Ordinal(k, n); }

    // ---- Сравнение ----
    bool operator==(const Ordinal& o) const { return k == o.k && n == o.n; }
    bool operator!=(const Ordinal& o) const { return !(*this == o); }

    bool operator<(const Ordinal& o) const {
        return k != o.k ? k < o.k : n < o.n;
    }
    bool operator<=(const Ordinal& o) const { return !(o < *this); }
    bool operator> (const Ordinal& o) const { return o < *this;    }
    bool operator>=(const Ordinal& o) const { return !(*this < o); }

    // ---- Арифметика ----
    Ordinal operator+(const Ordinal& o) const {
        return Ordinal(k + o.k, n + o.n);
    }

    // Следующий ординал
    Ordinal Next() const { return Ordinal(k, n + 1); }

    // Вывод
    std::string ToString() const {
        if (k == 0)  return std::to_string(n);
        if (k == 1)  return n == 0 ? "omega" : "omega+" + std::to_string(n);
        return "omega*" + std::to_string(k) + (n == 0 ? "" : "+" + std::to_string(n));
    }

    friend std::ostream& operator<<(std::ostream& os, const Ordinal& o) {
        return os << o.ToString();
    }
};

#endif // ORDINAL_H
