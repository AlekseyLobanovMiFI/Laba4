#ifndef STACK_H
#define STACK_H

#include "mutableArraySequence.h"

template<class T> class Stack {
private:
    MutableArraySequence<T> data;

public:
    void Push(const T& item){
        data.Append(item);
    }

    T Pop() {
        if (data.GetLength() == 0){
            throw std::out_of_range("Empty stack");
        }

        T val = data.GetLast();
        data.RemoveLast();
        return val;
    }

    T Top() const {
        return data.GetLast();
    }

    bool IsEmpty() const {
        return data.GetLength() == 0;
    }

    void Clear() {
        while (!IsEmpty()) {
            Pop();
        }
    }

    int Size() const {
        return data.GetLength();
    }

    T Get(int index) const {
        return data.Get(index);
    }
};

#endif
