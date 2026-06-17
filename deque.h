#ifndef DEQUE_H
#define DEQUE_H

#include <stdexcept>
#include "dynamicArray.h"

template<class T> class Deque {
private:
    static const int blockSize = 4;
    struct Block {
        T items[blockSize];
    };

    DynamicArray<Block> blocks;

    int headBlock, headIndex;
    int tailBlock, tailIndex;

    int size;

    void AddBlockBack() {
        int oldSize = blocks.GetSize();
        blocks.Resize(oldSize + 1);

    }

    void AddBlockFront() {
        int oldSize = blocks.GetSize();
        blocks.Resize(oldSize + 1);

        for (int i = oldSize; i > 0; i--) {
            blocks[i] = blocks[i - 1];
        }


        headBlock++;
        tailBlock++;
    }

public:
    Deque() : blocks(1), size(0) {
        headBlock = 0;
        headIndex = 0;

        tailBlock = 0;
        tailIndex = 0;
    }

    void PushBack(const T& item) {
        if (tailIndex == blockSize) {
            if (tailBlock == blocks.GetSize() - 1) {
                AddBlockBack();
            }
            tailBlock++;
            tailIndex = 0;
        }

        blocks[tailBlock].items[tailIndex] = item;
        tailIndex++;
        size++;
    }

    void PushFront(const T& item) {
        if (headIndex == 0) {
            if (headBlock == 0) {
                AddBlockFront();
            } else {
                headBlock--;
            }
            headIndex = blockSize;
        }

        headIndex--;
        blocks[headBlock].items[headIndex] = item;
        size++;
    }

    T PopFront() {
        if (size == 0) {
            throw std::out_of_range("Empty deque");
        }

        T val = blocks[headBlock].items[headIndex];

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
        T val = blocks[tailBlock].items[tailIndex];

        size--;
        return val;
    }


    T Front() const {
        if (size == 0) {
            throw std::out_of_range("Empty deque");
        }
        return blocks[headBlock].items[headIndex];
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
        return blocks[block].items[index];
    }

    T Get(int index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Index out of range");
        }

        int absoluteIndex = headIndex + index;

        int blockShift = absoluteIndex / blockSize;
        int innerIndex = absoluteIndex % blockSize;

        int block = headBlock + blockShift;

        return blocks[block].items[innerIndex];
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