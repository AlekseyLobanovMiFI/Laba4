#ifndef CARDINAL_H
#define CARDINAL_H

#include <iostream>
#include <stdexcept>
#include <string>

// ============================================================
//  Cardinal — множество {0, 1, 2, ..., ω}
//
//  ω — первое трансфинитное число (omega).
//  GetLength() у LazySequence возвращает Cardinal:
//    конечная последовательность  → Cardinal(n)
//    бесконечная последовательность → Cardinal::Omega()
// ============================================================

class Cardinal {
private:
    int value;          // >= 0 для натуральных чисел
    bool infinite;      // true → это ω

    Cardinal(int v, bool inf) : value(v), infinite(inf) {}

public:
    // ---- Конструкторы ----

    // Из натурального числа: Cardinal(5), Cardinal(0)
    Cardinal(int n) : value(n), infinite(false) {
        if (n < 0) throw std::out_of_range("Cardinal: negative value");
    }

    // ---- Фабрики ----
    static Cardinal Omega()         { return Cardinal(0, true); }
    static Cardinal FromInt(int n)  { return Cardinal(n); }

    // ---- Запросы ----
    bool IsFinite()   const { return !infinite; }
    bool IsInfinite() const { return  infinite; }

    // Получить числовое значение (только для конечных)
    int ToInt() const {
        if (infinite)
            throw std::logic_error("Cardinal: omega has no integer value");
        return value;
    }

    // ---- Операторы сравнения ----
    bool operator==(const Cardinal& o) const {
        if (infinite && o.infinite) return true;
        if (infinite || o.infinite) return false;
        return value == o.value;
    }
    bool operator!=(const Cardinal& o) const { return !(*this == o); }

    bool operator<(const Cardinal& o) const {
        if (infinite)   return false;       // ω не меньше ничего
        if (o.infinite) return true;        // n < ω для любого n
        return value < o.value;
    }
    bool operator<=(const Cardinal& o) const { return !(o < *this); }
    bool operator> (const Cardinal& o) const { return o < *this;    }
    bool operator>=(const Cardinal& o) const { return !(*this < o); }

    // ---- Арифметика ----
    Cardinal operator+(const Cardinal& o) const {
        if (infinite || o.infinite) return Omega();
        return Cardinal(value + o.value);
    }

    // ---- Вывод ----
    std::string ToString() const {
        return infinite ? "omega" : std::to_string(value);
    }

    friend std::ostream& operator<<(std::ostream& os, const Cardinal& c) {
        return os << c.ToString();
    }
};

#endif // CARDINAL_H
