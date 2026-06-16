#include "mutableArraySequence.h"
#include "queue.h"

template <class T> class LazySequence;
template <class T> class Generator
{
private:
    LazySequence<T>* owner;

    struct Change
    {
        enum Kind{Paste,Remove};

        Kind change;
        int index;
        LazySequence<T> elems;

        Change() : change(Remove), index(0), elems(){}

        Change(Kind kind, int i): change(kind), index(i), elems(){}

        Change(Kind kind, int i, const LazySequence<T>& values): change(kind), index(i), elems(values){}
    };

    Queue<Change> changes;

    Sequence<T>* source;
    std::function<T(Sequence<T>*)> rule;

    int position;

public:
    Generator(LazySequence<T>* o, std::function<T(Sequence<T>*)> r)
    {
        owner = o;
        source = nullptr;
        rule = r;
        position = 0;
    }

    Generator(LazySequence<T>* o, Sequence<T>* s)
    {
        owner = o;
        source = s;
        rule = nullptr;
        position = 0;
    }

    T GetNext()
    {
        if (source != nullptr)
        {
            if (position >= source->GetLength())
            {
                throw std::out_of_range("End of source");
            }

            T item = source->Get(position);
            position++;

            return item;
        }

        if (rule)
        {
            T item = rule(&(owner->cache));
            position++;

            return item;
        }

        throw std::out_of_range("Generator has no source or rule");
    }

    bool HasNext() const
    {
        if (source != nullptr)
        {
            return position < source->GetLength();
        }

        if (rule)
        {
            return true;
        }

        return false;
    }

    void AddPaste(int index, const LazySequence<T>& values)
    {
        Change newChange(Change::Paste, index, values);
        changes.Enqueue(newChange);
    }

    void AddRemove(int index)
    {
        Change newChange(Change::Remove, index);
        changes.Enqueue(newChange);
    }
};


template <class T> class LazySequence
{
friend class Generator<T>;
private:
    MutableArraySequence<T> cache;
    int sourceLength;

    Generator<T> generator;

public:
    LazySequence();

    LazySequence(const T* items, int count);

    LazySequence(Sequence<T>* source);

    LazySequence(std::function<T(Sequence<T>*)> rule, Sequence<T>* start);

    LazySequence(const LazySequence<T>& other);

    LazySequence<T>& operator=(const LazySequence<T>& other);

    ~LazySequence();

    T GetFirst() const;

    T GetLast() const;

    T Get(int index) const;

    LazySequence<T> GetSubsequence(int startIndex, int endIndex) const;

    int GetLength() const;

    LazySequence<T>* Append(const T& item);

    LazySequence<T>* Prepend(const T& item);

    LazySequence<T>* InsertAt(const T& item, int index);


};