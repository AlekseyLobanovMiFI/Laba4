#ifndef DEQUE_H
#define DEQUE_H

#include <stdexcept>
#include "dynamicArray.h"

template<class T> class Deque {
private:
    static const int blockSize = 2;

    struct Block{
        T items[blockSize];

        T& operator[](int i){
            return items[i];
        }

        const T& operator[](int i) const{
            return items[i];
        }
    };

    MutableArraySequence<Block> blocks;

    int headBlock, headIndex;
    int tailBlock, tailIndex;

    int size;

public:
    Deque() : blocks(0), size(0) {
        headBlock = 0;
        headIndex = 0;

        tailBlock = -1;
        tailIndex = blockSize;
    }

    void PushBack(const T& item) {
        if (tailIndex == blockSize) {
            if (tailBlock == blocks.GetLength() - 1) {
                blocks.Append(Block());
            }
            tailBlock++;
            tailIndex = 0;
        }

        blocks[tailBlock][tailIndex] = item;
        tailIndex++;
        size++;
    }

    void PushFront(const T& item) {
        if (headIndex == 0) {
            if (headBlock == 0) {
                blocks.Prepend(Block());
                tailBlock++;
            } else {
                headBlock--;
            }
            headIndex = blockSize;
        }

        headIndex--;
        blocks[headBlock][headIndex] = item;
        size++;
    }

    T PopFront() {
        if (size == 0) {
            throw std::out_of_range("Empty deque");
        }

        T val = blocks[headBlock][headIndex];

        headIndex++;
        size--;

        if (headIndex == blockSize) {
            headIndex = 0;
            headBlock++;
        }

        return val;
    }

    T PopBack() {
        if (size == 0) {
            throw std::out_of_range("Empty deque");
        }

        if (tailIndex == 0) {
            tailBlock--;
            tailIndex = blockSize;
        }

        tailIndex--;
        T val = blocks[tailBlock][tailIndex];

        size--;
        return val;
    }


    T Front() const {
        if (size == 0) {
            throw std::out_of_range("Empty deque");
        }
        return blocks[headBlock][headIndex];
    }

    T Back() const {
        if (size == 0) {
            throw std::out_of_range("Empty deque");
        }

        int block = tailBlock;
        int index = tailIndex;

        if (index == 0) {
            block--;
            index = blockSize;
        }

        index--;
        return blocks[block][index];
    }

    T Get(int index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Index out of range");
        }

        int absoluteIndex = headIndex + index;

        int blockShift = absoluteIndex / blockSize;
        int innerIndex = absoluteIndex % blockSize;

        int block = headBlock + blockShift;

        return blocks[block][innerIndex];
    }

    bool IsEmpty() const {
        return size == 0;
    }

    int Size() const {
        return size;
    }

    void Clear() {
        headBlock = 0;
        headIndex = 0;
        tailBlock = 0;
        tailIndex = 0;
        size = 0;
    }
};

#endif