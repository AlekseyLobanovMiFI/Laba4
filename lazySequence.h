#ifndef LAZY_SEQUENCE_H
#define LAZY_SEQUENCE_H

#include <stdexcept>
#include <functional>
#include <climits>
#include "sequence.h"
#include "cardinal.h"
#include "ordinal.h"
#include "mutableArraySequence.h"
#include "queue.h"

template <class T> class LazySequence;

// ============================================================
//  IGenerator<T> — абстрактный базовый класс генератора
//
//  Содержит общую для всех генераторов очередь изменений
//  (Change::Paste / Change::Remove) с физическими индексами.
//  Конкретные реализации различаются только способом
//  порождения физических элементов (GetNext/HasNext).
//
//  Это соответствует принципу неизменности ленивых
//  последовательностей: правило работает по физическому кэшу
//  и не «знает» о логических изменениях.
// ============================================================

template <class T>
class IGenerator {
    friend class LazySequence<T>;

public:
    static constexpr int INF = -1;//сигнал что длина бесконечна
    static constexpr int INVALID_IDX = INT_MIN;// невозможно получить нормальный физический индекс

protected:
    // ---- Очередь изменений ----
    // Хранится здесь, чтобы любой тип генератора поддерживал

    struct Change {
        enum Kind { Paste, Remove } kind;
        int index;  // физический индекс
        union {
            LazySequence<T> elems;  // Paste: вставляемая последовательность (владеем)
            int count;  // Remove: сколько удалить
        };

        Change() : kind(Remove), index(0), count(0) {}
        ~Change() { if (kind == Paste) elems.~LazySequence<T>(); }

        Change(const Change& other) : kind(other.kind), index(other.index) {
            if (kind == Paste)
                new (&elems) LazySequence<T>(other.elems);//создать объект по адресу
            else
                count = other.count;
        }

        Change& operator=(const Change& other) {
            if (this == &other) return *this;
            if (kind == Paste) elems.~LazySequence<T>();
            kind  = other.kind;
            index = other.index;
            if (kind == Paste)
                new (&elems) LazySequence<T>(other.elems);
            else
                count = other.count;
            return *this;
        }

        int Count() const;  // реализация после LazySequence
    };

    Queue<Change> changes;
    int splitChangeIdx    = -1;
    int splitChangeOffset = -1;

    // ---- Перевод логического -> физический ----
    struct PhysResult { bool isSplit; int index; };//попали во внутрь вставки

    PhysResult userToPhys(int userIdx) {
        int current = userIdx;
        for (int i = 0; i < changes.GetSize(); i++) {
            Change c = changes.Get(i);
            if (c.kind == Change::Paste) {
                int cnt = c.Count();
                bool inside = (cnt == INF)?(current >= c.index):(current >= c.index && current < c.index + cnt);
                if (inside) {
                    splitChangeIdx    = i;//какой paste разрезать
                    splitChangeOffset = current - c.index;//где конкретно разрезать
                    return {true, INVALID_IDX};
                }
                if (cnt != INF && current >= c.index + cnt)
                    current -= cnt;
            } else {
                if (current >= c.index && current < c.index + c.count)
                    throw std::out_of_range("IndexOutOfRange");
                if (current >= c.index)
                    current += c.count;
            }
        }
        return {false, current};
    }

    void splitAndPaste(LazySequence<T> newItems);  // после LazySequence

    // Суммарное изменение длины от изменений
    int Delta() const {
        int delta = 0;
        for (int i = 0; i < changes.GetSize(); i++) {
            Change c = changes.Get(i);
            if (c.kind == Change::Paste) {
                if (c.Count() == INF) return INF;
                delta += c.Count();
            } else {
                delta -= c.count;
            }
        }
        return delta;
    }

    // Применяет изменения к физическому индексу для GetAt
    // Возвращает элемент если он в Paste-блоке, иначе корректирует phys
    struct GetAtResult { bool found; T value; int phys; };

    GetAtResult applyChanges(int phys) {//поиск элементов
        for (int i = 0; i < changes.GetSize(); i++) {
            Change c = changes.Get(i);
            if (c.kind == Change::Paste) {
                int cnt = c.Count();
                if (cnt == INF) {
                    if (phys >= c.index)
                        return {true, const_cast<LazySequence<T>&>(c.elems).Get(phys - c.index), 0};
                } else {
                    if (phys >= c.index && phys < c.index + cnt)
                        return {true, const_cast<LazySequence<T>&>(c.elems).Get(phys - c.index), 0};
                    if (phys >= c.index + cnt)
                        phys -= cnt;
                }
            } else {
                if (phys >= c.index) {
                    phys += c.count;
                    if (phys < c.index + c.count)
                        throw std::out_of_range("IndexOutOfRange");
                }
            }
        }
        return {false, T{}, phys};
    }

public:
    virtual ~IGenerator() = default;

    // Копирование изменений при клонировании
    IGenerator() = default;
    IGenerator(const IGenerator&) = default;
    IGenerator& operator=(const IGenerator&) = default;

    // ---- Абстрактные методы ----
    virtual T        GetNext()              = 0;
    virtual bool     HasNext()        const = 0;
    virtual Cardinal BaseLength()     const = 0;  // длина без изменений
    virtual IGenerator<T>* Clone()    const = 0;
    virtual void     SetOwner(LazySequence<T>*) = 0;

    // ---- Конкретные методы (общие для всех) ----

    Cardinal GetLength() const {
        Cardinal base = BaseLength();
        if (base.IsInfinite()) {
            // Бесконечная: изменения могут сделать её ещё «длиннее» (другой Paste),
            // но базовая бесконечность сохраняется
            return Cardinal::Omega();
        }
        int delta = Delta();
        if (delta == INF) return Cardinal::Omega();
        return Cardinal(base.ToInt() + delta);
    }

    bool IsInfinite() const { return GetLength().IsInfinite(); }

    virtual T GetAt(Ordinal idx) {
        if (idx.k != 0)
            throw std::out_of_range("GetAt: transfinite index requires ConcatGenerator");
        int phys = idx.n;
        GetAtResult r = applyChanges(phys);
        if (r.found) return r.value;
        while (!ownedCacheFull(r.phys))
            GetNext();
        return ownerCache()->Get(r.phys);
    }

    // ---- Добавление изменений ----

    void AddPaste(int userIdx, LazySequence<T> newItems) {
        PhysResult r = userToPhys(userIdx);
        if (r.isSplit) {
            splitAndPaste(std::move(newItems));
        } else {
            Change c;
            c.kind  = Change::Paste;
            c.index = r.index;
            new (&c.elems) LazySequence<T>(std::move(newItems));
            changes.Enqueue(c);
        }
    }

    void AddRemove(int userIdx, int count) {
        if (count <= 0) return;
        PhysResult r = userToPhys(userIdx);
        if (r.isSplit) {
            // Удаляем элементы внутри Paste-блока — разбиваем блок
            Change patched = changes.Get(splitChangeIdx);
            int endOffset = splitChangeOffset + count;
            int blockSize = patched.Count();
            // Перестраиваем elems без удалённого диапазона
            LazySequence<T> newElems;
            if (splitChangeOffset > 0) {
                LazySequence<T>* left = patched.elems.GetSubsequence(0, splitChangeOffset - 1);
                newElems = *left;
                delete left;
            }
            if (blockSize == INF || endOffset < blockSize) {
                int rightStart = endOffset;
                if (blockSize == INF) {
                    // Бесконечный хвост после удалённого диапазона
                    LazySequence<T> tail(patched.elems);
                    tail.Remove(0, endOffset);
                    newElems.Concat(tail);
                } else {
                    LazySequence<T>* right = patched.elems.GetSubsequence(rightStart, blockSize - 1);
                    newElems.Concat(*right);
                    delete right;
                }
            }
            // Заменяем старый блок новым
            Queue<Change> newChanges;
            for (int i = 0; i < changes.GetSize(); i++) {
                if (i == splitChangeIdx) {
                    if (newElems.GetLength() == Cardinal(0)) {
                        // Блок пустой — просто пропускаем
                    } else {
                        Change upd;
                        upd.kind  = Change::Paste;
                        upd.index = changes.Get(i).index;
                        new (&upd.elems) LazySequence<T>(std::move(newElems));
                        newChanges.Enqueue(upd);
                    }
                } else {
                    newChanges.Enqueue(changes.Get(i));
                }
            }
            changes = newChanges;
            splitChangeIdx = splitChangeOffset = -1;
            return;
        }
        Change c;
        c.kind  = Change::Remove;
        c.index = r.index;
        c.count = count;
        changes.Enqueue(c);
    }

private:
    // Доступ к кэшу владельца — реализуется через виртуальные методы
    virtual bool ownedCacheFull(int phys) const = 0;//пока нужный элемент ещё не материализован, генерируем следующий
    virtual MutableArraySequence<T>* ownerCache() const = 0;//генератор должен вернуть указатель на кэш владельца, то есть на cache внутри LazySequence
};

// ============================================================
//  RuleGenerator<T>
//  Бесконечная последовательность с рекуррентным правилом.
// ============================================================

template <class T>
class RuleGenerator : public IGenerator<T> {
    LazySequence<T>*                     owner;
    std::function<T(const Sequence<T>*)> rule;

public:
    RuleGenerator(LazySequence<T>* owner,
                  std::function<T(const Sequence<T>*)> rule)
        : owner(owner), rule(rule) {}

    void SetOwner(LazySequence<T>* o) override { owner = o; }

    T GetNext() override {
        T val = rule(&owner->cache);
        owner->cache.Append(val);
        return val;
    }

    bool HasNext() const override { return true; }

    Cardinal BaseLength() const override { return Cardinal::Omega(); }

    IGenerator<T>* Clone() const override {//полиморфное копирование хотим знать какой конкертно вид генератора вызван
        auto* g = new RuleGenerator<T>(owner, rule);
        g->changes = this->changes;
        return g;
    }

private:
    bool ownedCacheFull(int phys) const override {
        return owner->cache.GetLength() > phys;
    }
    MutableArraySequence<T>* ownerCache() const override {
        return &owner->cache;
    }
};

// ============================================================
//  SourceGenerator<T>
//  Конечная последовательность из Sequence* или кэша.
// ============================================================

template <class T>
class SourceGenerator : public IGenerator<T> {
    LazySequence<T>*   owner;
    const Sequence<T>* source;

public:
    SourceGenerator(LazySequence<T>* owner, const Sequence<T>* src)
        : owner(owner), source(src) {}

    SourceGenerator(LazySequence<T>* owner, std::nullptr_t)
        : owner(owner), source(nullptr) {}

    void SetOwner(LazySequence<T>* o) override { owner = o; }

    T GetNext() override {
        int next = owner->cache.GetLength();
        if (!source || next >= source->GetLength())
            throw std::out_of_range("IndexOutOfRange");
        T val = source->Get(next);
        owner->cache.Append(val);
        return val;
    }

    bool HasNext() const override {
        if (source) return owner->cache.GetLength() < source->GetLength();
        return false;
    }

    Cardinal BaseLength() const override {
        int base = source ? source->GetLength() : owner->cache.GetLength();
        return Cardinal(base);
    }

    IGenerator<T>* Clone() const override {
        auto* g = new SourceGenerator<T>(owner, source);
        g->changes = this->changes;
        return g;
    }

private:
    bool ownedCacheFull(int phys) const override {
        return owner->cache.GetLength() > phys;
    }
    MutableArraySequence<T>* ownerCache() const override {
        return &owner->cache;
    }
};

// ============================================================
//  ConcatGenerator<T>
//  Склейка нескольких LazySequence.
//  Get(ω·k + n) → n-й элемент k-й последовательности.
// ============================================================

template <class T>
class ConcatGenerator : public IGenerator<T> {
    MutableArraySequence<LazySequence<T>*> seqs;   // не владеем (Add)
    MutableArraySequence<LazySequence<T>*> owned;  // владеем (AddOwned)

public:
    ConcatGenerator() = default;

    ~ConcatGenerator() {
        for (int i = 0; i < owned.GetLength(); i++)
            delete owned.Get(i);
    }

    void Add(LazySequence<T>* seq) { seqs.Append(seq); }
    int  BlockCount() const        { return seqs.GetLength(); }

    // Добавить последовательность с передачей владения
    void AddOwned(LazySequence<T>* seq) {
        seqs.Append(seq);
        owned.Append(seq);
    }

    void SetOwner(LazySequence<T>*) override {}  // не нужен

    T GetNext() override {
        throw std::logic_error("ConcatGenerator: use GetAt(Ordinal) instead");
    }

    bool HasNext() const override { return true; }

    Cardinal BaseLength() const override {
        // Если все бесконечные — Omega, иначе сумма
        for (int i = 0; i < seqs.GetLength(); i++)
            if (seqs.Get(i)->IsInfinite()) return Cardinal::Omega();
        int sum = 0;
        for (int i = 0; i < seqs.GetLength(); i++)
            sum += seqs.Get(i)->GetLength().ToInt();
        return Cardinal(sum);
    }

    // Переопределяем GetAt для трансфинитных индексов
    T GetAt(Ordinal idx) override {
        if (idx.k >= seqs.GetLength())
            throw std::out_of_range("ConcatGenerator: block index out of range");
        return seqs.Get(idx.k)->Get(idx.n);
    }

    // Взять подпоследовательность по трансфинитным индексам.
    // Возвращает ленивую конечную LazySequence.
    LazySequence<T>* GetSubsequence(Ordinal from, Ordinal to) {
        if (to < from)
            throw std::out_of_range("GetSubsequence: 'to' < 'from'");
        if (from.k >= seqs.GetLength() || to.k >= seqs.GetLength())
            throw std::out_of_range("GetSubsequence: block index out of range");

        if (from.k == to.k) {
            // Один блок — обычный GetSubsequence внутри него
            return seqs.Get(from.k)->GetSubsequence(from.n, to.n);
        }

        // Несколько блоков — собираем через ConcatGenerator
        auto* cg = new ConcatGenerator<T>();

        // Первый блок: от from.n до конца (ленивый хвост через InsertAt + Remove)
        {
            LazySequence<T>* first = new LazySequence<T>(*seqs.Get(from.k));
            if (from.n > 0) first->Remove(0, from.n);
            cg->AddOwned(first);
        }

        // Средние блоки: целиком
        for (int k = from.k + 1; k < to.k; k++) {
            LazySequence<T>* mid = new LazySequence<T>(*seqs.Get(k));
            cg->AddOwned(mid);
        }

        // Последний блок: от 0 до to.n
        {
            LazySequence<T>* last = seqs.Get(to.k)->GetSubsequence(0, to.n);
            cg->AddOwned(last);
        }

        return new LazySequence<T>(cg);
    }

    IGenerator<T>* Clone() const override {
        auto* g = new ConcatGenerator<T>();
        // Клонируем owned последовательности
        for (int i = 0; i < owned.GetLength(); i++) {
            LazySequence<T>* c = new LazySequence<T>(*owned.Get(i));
            g->owned.Append(c);
        }
        // seqs содержит и owned и внешние — нужно заменить owned на клоны
        for (int i = 0; i < seqs.GetLength(); i++) {
            bool isOwned = false;
            for (int j = 0; j < owned.GetLength(); j++) {
                if (seqs.Get(i) == owned.Get(j)) {
                    g->seqs.Append(g->owned.Get(j));
                    isOwned = true;
                    break;
                }
            }
            if (!isOwned) g->seqs.Append(seqs.Get(i));
        }
        g->changes = this->changes;
        return g;
    }

private:
    bool ownedCacheFull(int) const override { return true; }
    MutableArraySequence<T>* ownerCache() const override { return nullptr; }
};

// ============================================================
//  LazySequence<T>
// ============================================================

template <class T>
class LazySequence {
    friend class IGenerator<T>;
    friend class RuleGenerator<T>;
    friend class SourceGenerator<T>;

private:
    MutableArraySequence<T> cache;
    IGenerator<T>*          gen;   // владеем

    void reconnectOwner() {//нужен после копирования LazySequence, чтобы генератор новой последовательности ссылался на новый объект, а не на старый
        rg->SetOwner(this);
        if (auto* sg = dynamic_cast<SourceGenerator<T>*>(gen))
            sg->SetOwner(this);
    }

public:
    // ======================== Конструкторы ========================

    LazySequence()
        : gen(new SourceGenerator<T>(this, nullptr)) {}

    LazySequence(const T* items, int count)
        : cache(count < 0 ? throw std::out_of_range("NegativeCount"), nullptr : items, count),
          gen(new SourceGenerator<T>(this, nullptr))
    {}

    explicit LazySequence(const Sequence<T>* seq)//запрещает неявное преобр Seq в LazySeq
        : gen(new SourceGenerator<T>(this, seq))
    {}

    LazySequence(T(*func)(const Sequence<T>*), const Sequence<T>* seeds)//seeds - задаток новой послед(типо 1 1 для фиб)
        : cache(seeds ? seeds->GetLength() : 0),
          gen(new RuleGenerator<T>(this, std::function<T(const Sequence<T>*)>(func)))
    {
        if (seeds)
            for (int i = 0; i < seeds->GetLength(); i++)
                cache[i] = seeds->Get(i);
    }

    LazySequence(std::function<T(const Sequence<T>*)> func, const Sequence<T>* seeds)
        : cache(seeds ? seeds->GetLength() : 0),
          gen(new RuleGenerator<T>(this, func))
    {
        if (seeds)
            for (int i = 0; i < seeds->GetLength(); i++)
                cache[i] = seeds->Get(i);
    }

    LazySequence(std::function<T(const Sequence<T>*)> func,
                 const T* seeds, int seedCount)
        : cache(seedCount < 0 ? throw std::out_of_range("NegativeCount"), nullptr : seeds, seedCount),
          gen(new RuleGenerator<T>(this, func))
    {}

    explicit LazySequence(ConcatGenerator<T>* cg)//создаёт ленивую последовательность из уже готового ConcatGenerator
        : gen(cg)
    {}

    LazySequence(const LazySequence<T>& other)
        : cache(other.cache), gen(other.gen->Clone())
    {
        reconnectOwner();
    }

    LazySequence<T>& operator=(const LazySequence<T>& other) {
        if (this == &other) return *this;
        delete gen;
        cache = other.cache;
        gen   = other.gen->Clone();
        reconnectOwner();
        return *this;
    }

    ~LazySequence() { delete gen; }

    // ======================== Декомпозиция ========================

    T GetFirst() { return Get(Ordinal(0)); }

    T GetLast() {
        Cardinal len = gen->GetLength();
        if (len.IsInfinite())
            throw std::logic_error("GetLast undefined for infinite sequence");
        if (len.ToInt() == 0)
            throw std::out_of_range("IndexOutOfRange");
        return Get(Ordinal(len.ToInt() - 1));
    }

    T Get(Ordinal idx) {
        if (idx.IsFinite()) {
            Cardinal len = gen->GetLength();
            if (Cardinal(idx.n) >= len)
                throw std::out_of_range("IndexOutOfRange");
        }
        return gen->GetAt(idx);
    }

    T Get(int logIdx) { return Get(Ordinal(logIdx)); }

    Cardinal GetLength()          { return gen->GetLength(); }
    int  GetMaterializedCount()   const { return cache.GetLength(); }
    bool IsInfinite()             const { return gen->IsInfinite(); }

    LazySequence<T>* GetSubsequence(int startIndex, int endIndex) {//создаёт новую ленивую последовательность из диапазона элементов текущей
        if (startIndex < 0 || startIndex > endIndex)
            throw std::out_of_range("IndexOutOfRange");
        Cardinal len = gen->GetLength();
        if (Cardinal(endIndex) >= len)
            throw std::out_of_range("IndexOutOfRange");
        int count = endIndex - startIndex + 1;
        T* buf = new T[count];
        for (int i = 0; i < count; i++) buf[i] = Get(startIndex + i);
        LazySequence<T>* result = new LazySequence<T>(buf, count);
        delete[] buf;
        return result;
    }

    // ======================== Операции ============================
    // работают для любого типа генератора

    LazySequence<T>* Append(const T& item) {
        if (IsInfinite())
            throw std::logic_error("Append to infinite sequence undefined");
        gen->AddPaste(gen->GetLength().ToInt(), LazySequence<T>(&item, 1));
        return this;
    }

    LazySequence<T>* Append(const Sequence<T>* seq) {
        if (!seq || seq->GetLength() == 0) return this;
        if (IsInfinite())
            throw std::logic_error("Append to infinite sequence undefined");
        gen->AddPaste(gen->GetLength().ToInt(), LazySequence<T>(seq));
        return this;
    }

    LazySequence<T>* Prepend(const T& item)           { return InsertAt(item, 0); }
    LazySequence<T>* Prepend(const Sequence<T>* seq)  { return InsertAt(seq, 0); }

    LazySequence<T>* InsertAt(const T& item, int index) {
        if (index < 0) throw std::out_of_range("IndexOutOfRange");
        if (Cardinal(index) > gen->GetLength())
            throw std::out_of_range("IndexOutOfRange");
        gen->AddPaste(index, LazySequence<T>(&item, 1));
        return this;
    }

    LazySequence<T>* InsertAt(const Sequence<T>* seq, int index) {
        if (!seq || seq->GetLength() == 0) return this;
        if (index < 0) throw std::out_of_range("IndexOutOfRange");
        if (Cardinal(index) > gen->GetLength())
            throw std::out_of_range("IndexOutOfRange");
        gen->AddPaste(index, LazySequence<T>(seq));
        return this;
    }

    LazySequence<T>* InsertAt(const LazySequence<T>& lazy, int index) {
        if (index < 0) throw std::out_of_range("IndexOutOfRange");
        if (Cardinal(index) > gen->GetLength())
            throw std::out_of_range("IndexOutOfRange");
        gen->AddPaste(index, lazy);
        return this;
    }

    LazySequence<T>* RemoveAt(int index) { return Remove(index, 1); }

    LazySequence<T>* Remove(int index, int count) {
        if (index < 0 || count <= 0) throw std::out_of_range("IndexOutOfRange");
        if (Cardinal(index + count) > gen->GetLength())
            throw std::out_of_range("IndexOutOfRange");
        gen->AddRemove(index, count);
        return this;
    }

    LazySequence<T>* Concat(LazySequence<T>* other) {
        if (!other) return this;
        if (IsInfinite() || dynamic_cast<ConcatGenerator<T>*>(gen)) {
            // Бесконечная или уже ConcatGenerator — добавляем блок
            ConcatGenerator<T>* cg = dynamic_cast<ConcatGenerator<T>*>(gen);//dynamic_cast пытается безопасно привести IGenerator* к ConcatGenerator*
            if (!cg) {
                // Первый Concat на бесконечной: создаём ConcatGenerator
                cg = new ConcatGenerator<T>();
                cg->AddOwned(new LazySequence<T>(*this));
                delete gen;
                gen = cg;
                cache = MutableArraySequence<T>();
            }
            cg->Add(other);
        } else {
            // Конечная: патч
            gen->AddPaste(gen->GetLength().ToInt(), *other);
        }
        return this;
    }

    LazySequence<T>* Concat(const LazySequence<T>& other) {
        return Concat(const_cast<LazySequence<T>*>(&other));
    }

    // GetSubsequence с трансфинитными индексами.
    // Если оба индекса конечные (k==0) — сводится к обычному GetSubsequence(int,int).
    LazySequence<T>* GetSubsequence(Ordinal from, Ordinal to) {
        if (from.IsFinite() && to.IsFinite())
            return GetSubsequence(from.n, to.n);
        ConcatGenerator<T>* cg = dynamic_cast<ConcatGenerator<T>*>(gen);
        if (!cg)
            throw std::logic_error("GetSubsequence(Ordinal,Ordinal): transfinite index requires ConcatGenerator");
        return cg->GetSubsequence(from, to);
    }

    // ======================== Map / Where / Reduce ================

    LazySequence<T>* Map(std::function<T(const T&)> func,
                         int count = IGenerator<T>::INF) {
        if (count == IGenerator<T>::INF) {
            if (IsInfinite()) throw std::logic_error("Specify count for infinite sequence");
            count = gen->GetLength().ToInt();
        } else if (!IsInfinite()) {
            int len = gen->GetLength().ToInt();
            if (count > len) count = len;
        }
        T* buf = new T[count];
        for (int i = 0; i < count; i++) buf[i] = func(Get(i));
        LazySequence<T>* r = new LazySequence<T>(buf, count);
        delete[] buf;
        return r;
    }

    LazySequence<T>* Where(std::function<bool(const T&)> func,
                           int count = IGenerator<T>::INF) {
        if (count == IGenerator<T>::INF) {
            if (IsInfinite()) throw std::logic_error("Specify count for infinite sequence");
            count = gen->GetLength().ToInt();
        } else if (!IsInfinite()) {
            int len = gen->GetLength().ToInt();
            if (count > len) count = len;
        }
        MutableArraySequence<T> tmp;
        for (int i = 0; i < count; i++) {
            T val = Get(i);
            if (func(val)) tmp.Append(val);
        }
        return new LazySequence<T>(tmp.GetLength() > 0 ? &tmp[0] : nullptr, tmp.GetLength());
    }

    T Reduce(T init, std::function<T(const T&, const T&)> func, int count = IGenerator<T>::INF) {
        if (count == IGenerator<T>::INF) {
            if (IsInfinite()) throw std::logic_error("Specify count for infinite sequence");
            count = gen->GetLength().ToInt();
        } else if (!IsInfinite()) {
            int len = gen->GetLength().ToInt();
            if (count > len) count = len;
        }
        T res = init;
        for (int i = 0; i < count; i++) res = func(res, Get(i));
        return res;
    }

    void Zip(LazySequence<T>* other, int count, std::function<void(const T&, const T&)> consumer) {//обработка пар двух последоватльеностей
        for (int i = 0; i < count; i++)
            consumer(Get(i), other->Get(i));
    }
};

// ============================================================
//  Реализация методов IGenerator<T> (после LazySequence)
// ============================================================

template <class T> 
int IGenerator<T>::Change::Count() const {
    if (kind == Remove) return count;
    Cardinal len = elems.gen->GetLength();
    return len.IsInfinite() ? IGenerator<T>::INF : len.ToInt();
}

template <class T>
void IGenerator<T>::splitAndPaste(LazySequence<T> newItems) {
    Change c      = changes.Get(splitChangeIdx);
    int    offset = splitChangeOffset;
    int    total  = c.Count();

    LazySequence<T> left;
    if (offset > 0) {
        LazySequence<T>* tmp = c.elems.GetSubsequence(0, offset - 1);
        left = *tmp;
        delete tmp;
    }

    LazySequence<T> right;
    if (total == INF) {
        LazySequence<T> tail(c.elems);
        tail.Remove(0, offset);
        right = std::move(tail);
    } else if (offset < total) {
        LazySequence<T>* tmp = c.elems.GetSubsequence(offset, total - 1);
        right = *tmp;
        delete tmp;
    }

    left.Concat(std::move(newItems));
    left.Concat(std::move(right));

    Queue<Change> newChanges;
    for (int i = 0; i < changes.GetSize(); i++) {
        if (i == splitChangeIdx) {
            Change upd;
            upd.kind  = Change::Paste;
            upd.index = changes.Get(i).index;
            new (&upd.elems) LazySequence<T>(std::move(left));
            newChanges.Enqueue(upd);
        } else {
            newChanges.Enqueue(changes.Get(i));
        }
    }
    changes = newChanges;
    splitChangeIdx = splitChangeOffset = -1;
}

#endif // LAZY_SEQUENCE_H
