#ifndef IMMUTABLE_ARRAY_SEQUENCE_H
#define IMMUTABLE_ARRAY_SEQUENCE_H

#include "arraySequence.h"

template<class T> class ImmutableArraySequence : public ArraySequence<T>{
private:

protected:
    virtual void AppendImple(const T& item) override{
        this->ResizeSelf(this->GetLength()+1);
        (*this)[this->GetLength()-1] = item;
        //operator[](GetLength()-1)=item;
    }

public:
    virtual ImmutableArraySequence<T>* Instance() override { 
        return new ImmutableArraySequence(*this); 
    }

    virtual ImmutableArraySequence<T>* CreateNewArray(const T* items, int cnt) const override{
        return new ImmutableArraySequence<T>(items, cnt);
    }

    virtual ImmutableArraySequence<T>* NewEmpty() const override {
        return new ImmutableArraySequence<T>();
    }

    ImmutableArraySequence (const T* items, int count) : ArraySequence<T>(items, count){};
    ImmutableArraySequence () : ArraySequence<T>(){};
    ImmutableArraySequence (const ImmutableArraySequence & seq) : ArraySequence<T>(seq){};
    ImmutableArraySequence (int cnt) : ArraySequence<T>(cnt){};

};

#endif