#ifndef STREAM_H
#define STREAM_H

#include <stdexcept>
#include <functional>
#include <fstream>
#include <sstream>
#include <string>
#include "sequence.h"
#include "lazySequence.h"
#include "mutableArraySequence.h"

// ============================================================
//  Исключения потока
// ============================================================

struct EndOfStreamException : std::runtime_error {
    EndOfStreamException() : std::runtime_error("EndOfStream") {}
};

struct StreamNotOpenException : std::runtime_error {
    StreamNotOpenException() : std::runtime_error("StreamNotOpen") {}
};

// ============================================================
//  ReadOnlyStream<T>
//
//  Поток только для чтения. Чтение Read() = получить первый
//  элемент и сдвинуть позицию вперёд.
//
//  Источники:
//    - Sequence<T>*        (из памяти, поддерживает Seek/GoBack)
//    - LazySequence<T>*    (из памяти, поддерживает Seek)
//    - файл + Deserializer (построчная десериализация)
// ============================================================

template <class T>
class ReadOnlyStream {
public:
    // Тип функции десериализации: строка -> T
    using Deserializer = std::function<T(const std::string&)>;

private:
    // --- источник из памяти ---
    Sequence<T>*      memSource   = nullptr;   // не владеем
    LazySequence<T>*  lazySource  = nullptr;   // не владеем
    int               memLen      = 0;

    // --- источник из файла ---
    std::string       filePath;
    Deserializer      deserializer;
    std::ifstream     fileStream;
    bool              fromFile    = false;

    // --- общее состояние ---
    size_t  position  = 0;
    bool    isOpen    = false;

    // история позиций для GoBack (только для in-memory)
    MutableArraySequence<size_t> history;

    // --- внутреннее чтение следующего элемента ---
    T readNext() {
        if (fromFile) {
            std::string line;
            if (!std::getline(fileStream, line))
                throw EndOfStreamException();
            return deserializer(line);
        } else if (lazySource != nullptr) {
            // LazySequence может быть бесконечной —
            // IsEndOfStream всегда false для неё
            return lazySource->Get((int)position);
        } else {
            if ((int)position >= memLen)
                throw EndOfStreamException();
            return memSource->Get((int)position);
        }
    }

public:
    // ---- Конструкторы ----

    // Из Sequence<T> (случайный доступ, Seek, GoBack)
    explicit ReadOnlyStream(Sequence<T>* seq)
        : memSource(seq), memLen(seq ? seq->GetLength() : 0) {}

    // Из LazySequence<T> (Seek, но не GoBack для бесконечной)
    explicit ReadOnlyStream(LazySequence<T>* lazy)
        : lazySource(lazy),
          memLen(lazy && !lazy->IsInfinite() ? lazy->GetLength().ToInt() : -1) {}

    // Из файла: каждая строка -> один элемент типа T
    ReadOnlyStream(const std::string& path, Deserializer deser)
        : filePath(path), deserializer(deser), fromFile(true) {}

    ~ReadOnlyStream() {
        if (fileStream.is_open()) fileStream.close();
    }

    // ---- Управление потоком ----

    void Open() {
        if (isOpen) return;
        if (fromFile) {
            fileStream.open(filePath);
            if (!fileStream.is_open())
                throw std::runtime_error("Cannot open file: " + filePath);
        }
        position = 0;
        isOpen   = true;
    }

    void Close() {
        if (fromFile && fileStream.is_open()) fileStream.close();
        isOpen = false;
    }

    // ---- Декомпозиция ----

    bool IsEndOfStream() const {
        if (!isOpen) return true;
        if (fromFile)       return !const_cast<std::ifstream&>(fileStream).good()
                                || const_cast<std::ifstream&>(fileStream).peek() == EOF;
        if (lazySource)     return lazySource->IsInfinite()
                                ? false
                                : (int)position >= lazySource->GetLength().ToInt();
        return (int)position >= memLen;
    }

    // Считать элемент и сдвинуть позицию
    T Read() {
        if (!isOpen)        throw StreamNotOpenException();
        if (IsEndOfStream()) throw EndOfStreamException();

        // сохраняем позицию для GoBack
        if (!fromFile) history.Append(position);

        T val = readNext();
        position++;
        return val;
    }

    size_t GetPosition() const { return position; }

    // Можно ли перемещаться без чтения?
    bool IsCanSeek() const {
        return !fromFile;  // файловый поток не поддерживает произвольный Seek
    }

    // Перейти на позицию index (только для in-memory)
    size_t Seek(size_t index) {
        if (!isOpen)     throw StreamNotOpenException();
        if (!IsCanSeek()) throw std::logic_error("Seek not supported for file stream");

        size_t maxPos = (lazySource && lazySource->IsInfinite())
                        ? index          // бесконечная: любая позиция допустима
                        : (size_t)(memLen < 0 ? index : memLen);

        if (index > maxPos) index = maxPos;  // зажимаем до ближайшей допустимой
        position = index;
        return position;
    }

    // Можно ли вернуться назад?
    bool IsCanGoBack() const {
        return !fromFile && history.GetLength() > 0;
    }

    // Вернуться на предыдущую позицию
    void GoBack() {
        if (!IsCanGoBack())
            throw std::logic_error("GoBack not supported or no history");
        // берём предыдущую позицию из истории
        size_t prev = history.Get(history.GetLength() - 1);
        // убираем последний элемент истории
        // (используем RemoveLast из MutableArraySequence через ArraySequence)
        // приходится пересоздать без последнего
        history.RemoveLast();
        position = prev;
    }
};

// ============================================================
//  WriteOnlyStream<T>
//
//  Поток только для записи.
//
//  Назначения:
//    - Sequence<T>*    (дописывает в конец)
//    - файл + Serializer (каждый элемент -> строка -> файл)
// ============================================================

template <class T>
class WriteOnlyStream {
public:
    using Serializer = std::function<std::string(const T&)>;

private:
    Sequence<T>*  memDest    = nullptr;  // не владеем
    std::string   filePath;
    Serializer    serializer;
    std::ofstream fileStream;
    bool          fromFile   = false;

    size_t position = 0;
    bool   isOpen   = false;

public:
    // ---- Конструкторы ----

    // В Sequence<T>
    explicit WriteOnlyStream(Sequence<T>* seq)
        : memDest(seq) {}

    // В файл: T -> строка, каждый элемент на новой строке
    WriteOnlyStream(const std::string& path, Serializer ser)
        : filePath(path), serializer(ser), fromFile(true) {}

    ~WriteOnlyStream() {
        if (fileStream.is_open()) fileStream.close();
    }

    // ---- Управление ----

    void Open() {
        if (isOpen) return;
        if (fromFile) {
            fileStream.open(filePath, std::ios::app);
            if (!fileStream.is_open())
                throw std::runtime_error("Cannot open file: " + filePath);
        }
        isOpen = true;
    }

    void Close() {
        if (fromFile && fileStream.is_open()) fileStream.close();
        isOpen = false;
    }

    // ---- Операции ----

    // Записать элемент, вернуть новую позицию
    size_t Write(const T& item) {
        if (!isOpen) throw StreamNotOpenException();

        if (fromFile) {
            fileStream << serializer(item) << "\n";
            if (!fileStream.good())
                throw std::runtime_error("Write error");
        } else {
            if (memDest == nullptr)
                throw std::runtime_error("No destination sequence");
            memDest->Append(item);
        }

        position++;
        return position;
    }

    size_t GetPosition() const { return position; }
};

// ============================================================
//  Вспомогательные функции-шаблоны
// ============================================================

// Скопировать N элементов из ReadOnlyStream в WriteOnlyStream
template <class T>
size_t StreamCopy(ReadOnlyStream<T>& src, WriteOnlyStream<T>& dst, int n = -1) {
    size_t count = 0;
    while (!src.IsEndOfStream()) {
        if (n >= 0 && (int)count >= n) break;
        dst.Write(src.Read());
        count++;
    }
    return count;
}

// Фильтрация потока: читаем из src, пишем в dst только элементы, 
// прошедшие предикат
template <class T>
size_t StreamFilter(ReadOnlyStream<T>& src, WriteOnlyStream<T>& dst,
                    std::function<bool(const T&)> pred, int n = -1) {
    size_t count = 0;
    while (!src.IsEndOfStream()) {
        if (n >= 0 && (int)count >= n) break;
        T val = src.Read();
        if (pred(val)) { dst.Write(val); count++; }
    }
    return count;
}

// Map из потока в поток
template <class T>
size_t StreamMap(ReadOnlyStream<T>& src, WriteOnlyStream<T>& dst,
                 std::function<T(const T&)> func, int n = -1) {
    size_t count = 0;
    while (!src.IsEndOfStream()) {
        if (n >= 0 && (int)count >= n) break;
        dst.Write(func(src.Read()));
        count++;
    }
    return count;
}

#endif // STREAM_H
