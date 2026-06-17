#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <cassert>

#include "dynamicArray.h"
#include "linkedList.h"
#include "sequence.h"
#include "mutableArraySequence.h"
#include "immutableArraySequence.h"
#include "listSequence.h"
#include "cardinal.h"
#include "lazySequence.h"
#include "stream.h"

using namespace std;

static int testsPassed = 0;
static int testsFailed = 0;

#define ASSERT(cond, name) \
    do { \
        if (cond) { cout << "[OK] " << name << "\n"; testsPassed++; } \
        else      { cout << "[FAIL] " << name << "\n"; testsFailed++; } \
    } while(0)

#define ASSERT_THROW(expr, name) \
    do { \
        bool threw = false; \
        try { expr; } catch (...) { threw = true; } \
        ASSERT(threw, name); \
    } while(0)

// ================================================================
//  ТЕСТЫ LazySequence
// ================================================================

void test_lazy_finite_from_array() {
    cout << "\n-- LazySeq: Finite from array --\n";
    int arr[] = {10, 20, 30, 40, 50};
    LazySequence<int> ls(arr, 5);
    ASSERT(ls.GetLength() == Cardinal(5),  "GetLength == 5");
    ASSERT(ls.GetFirst()  == 10, "GetFirst == 10");
    ASSERT(ls.GetLast()   == 50, "GetLast == 50");
    ASSERT(ls.Get(2)      == 30, "Get(2) == 30");
    ASSERT_THROW(ls.Get(-1),     "Get(-1) throws");
    ASSERT_THROW(ls.Get(5),      "Get(5) throws");
}

void test_lazy_infinite_fibonacci() {
    cout << "\n-- LazySeq: Infinite Fibonacci --\n";
    auto rule = [](const Sequence<int>* c) -> int {
        int n = c->GetLength();
        return c->Get(n-1) + c->Get(n-2);
    };
    int seeds[] = {0, 1};
    LazySequence<int> fib(rule, seeds, 2);
    ASSERT(fib.Get(0)  == 0,  "fib(0)==0");
    ASSERT(fib.Get(5)  == 5,  "fib(5)==5");
    ASSERT(fib.Get(10) == 55, "fib(10)==55");
    ASSERT(fib.Get(10) == 55, "fib(10) cached");
    ASSERT(fib.GetMaterializedCount() == 11, "11 materialized");
    ASSERT(fib.GetLength() == Cardinal::Omega(), "GetLength of infinite == omega");
}

void test_lazy_mutations() {
    cout << "\n-- LazySeq: Mutations --\n";
    int arr[] = {1, 2, 3};
    LazySequence<int> ls(arr, 3);
    ls.Prepend(0);
    ASSERT(ls.GetLength() == Cardinal(4) && ls.GetFirst() == 0, "Prepend");
    ls.Append(4);
    ASSERT(ls.GetLength() == Cardinal(5) && ls.GetLast() == 4,  "Append");
    ls.InsertAt(99, 2);
    ASSERT(ls.GetLength() == Cardinal(6) && ls.Get(2) == 99,    "InsertAt");
}

void test_lazy_functional() {
    cout << "\n-- LazySeq: Map/Where/Reduce --\n";
    int arr[] = {1, 2, 3, 4, 5};
    LazySequence<int> ls(arr, 5);
    auto m = ls.Map([](const int& x){ return x*2; });
    ASSERT(m->Get(4) == 10, "Map *2");
    delete m;
    auto w = ls.Where([](const int& x){ return x > 2; });
    ASSERT(w->GetLength() == Cardinal(3) && w->Get(0) == 3, "Where >2");
    delete w;
    int s = ls.Reduce(0, [](const int& a, const int& b){ return a+b; });
    ASSERT(s == 15, "Reduce sum");
}

// ================================================================
//  ТЕСТЫ ReadOnlyStream
// ================================================================

void test_ros_from_sequence() {
    cout << "\n-- ReadOnlyStream from Sequence --\n";
    int arr[] = {10, 20, 30};
    MutableArraySequence<int> seq(arr, 3);
    ReadOnlyStream<int> ros(&seq);
    ros.Open();
    ASSERT(!ros.IsEndOfStream(),   "Not EOS at start");
    ASSERT(ros.GetPosition() == 0, "Position == 0");
    ASSERT(ros.Read() == 10,       "Read==10");
    ASSERT(ros.Read() == 20,       "Read==20");
    ASSERT(ros.Read() == 30,       "Read==30");
    ASSERT(ros.IsEndOfStream(),    "EOS after all read");
    ASSERT_THROW(ros.Read(),       "Read past EOS throws");
    ros.Close();
}

void test_ros_seek() {
    cout << "\n-- ReadOnlyStream: Seek --\n";
    int arr[] = {1, 2, 3, 4, 5};
    MutableArraySequence<int> seq(arr, 5);
    ReadOnlyStream<int> ros(&seq);
    ros.Open();
    ASSERT(ros.IsCanSeek(), "IsCanSeek==true");
    ros.Seek(3);
    ASSERT(ros.GetPosition() == 3, "Seek(3): pos==3");
    ASSERT(ros.Read() == 4,        "Read after Seek==4");
    ros.Close();
}

void test_ros_goback() {
    cout << "\n-- ReadOnlyStream: GoBack --\n";
    int arr[] = {5, 10, 15};
    MutableArraySequence<int> seq(arr, 3);
    ReadOnlyStream<int> ros(&seq);
    ros.Open();
    ros.Read(); // 5
    ros.Read(); // 10
    ASSERT(ros.IsCanGoBack(), "IsCanGoBack==true");
    ros.GoBack();
    ASSERT(ros.GetPosition() == 1, "After GoBack: pos==1");
    ASSERT(ros.Read() == 10,       "Re-read==10");
    ros.Close();
}

void test_ros_from_lazy() {
    cout << "\n-- ReadOnlyStream from LazySequence (infinite Fib) --\n";
    auto rule = [](const Sequence<int>* c) -> int {
        int n = c->GetLength();
        return c->Get(n-1) + c->Get(n-2);
    };
    int seeds[] = {0, 1};
    LazySequence<int> fib(rule, seeds, 2);
    ReadOnlyStream<int> ros(&fib);
    ros.Open();
    ASSERT(!ros.IsEndOfStream(), "Infinite: not EOS");
    ASSERT(ros.Read() == 0, "fib[0]==0");
    ASSERT(ros.Read() == 1, "fib[1]==1");
    ASSERT(ros.Read() == 1, "fib[2]==1");
    ASSERT(ros.Read() == 2, "fib[3]==2");
    ASSERT(ros.Read() == 3, "fib[4]==3");
    ASSERT(ros.GetPosition() == 5, "Position==5");
    ros.Close();
}

void test_ros_from_file() {
    cout << "\n-- ReadOnlyStream from file --\n";
    const string path = "/tmp/test_stream_ro.txt";
    { ofstream f(path); f << "42\n17\n99\n"; }

    ReadOnlyStream<int> ros(path, [](const string& s){ return stoi(s); });
    ros.Open();
    ASSERT(ros.Read() == 42,       "File Read==42");
    ASSERT(ros.Read() == 17,       "File Read==17");
    ASSERT(ros.Read() == 99,       "File Read==99");
    ASSERT(ros.IsEndOfStream(),    "File EOS");
    ASSERT(!ros.IsCanSeek(),       "File IsCanSeek==false");
    ros.Close();
}

// ================================================================
//  ТЕСТЫ WriteOnlyStream
// ================================================================

void test_wos_to_sequence() {
    cout << "\n-- WriteOnlyStream to Sequence --\n";
    MutableArraySequence<int> seq;
    WriteOnlyStream<int> wos(&seq);
    wos.Open();
    wos.Write(1); wos.Write(2); wos.Write(3);
    ASSERT(wos.GetPosition() == 3, "WOS position==3");
    ASSERT(seq.GetLength()   == 3, "seq length==3");
    ASSERT(seq.Get(0) == 1,        "seq[0]==1");
    ASSERT(seq.Get(2) == 3,        "seq[2]==3");
    wos.Close();
}

void test_wos_to_file() {
    cout << "\n-- WriteOnlyStream to file --\n";
    const string path = "/tmp/test_stream_wo.txt";
    { ofstream f(path, ios::trunc); }

    WriteOnlyStream<int> wos(path, [](const int& x){ return to_string(x); });
    wos.Open();
    wos.Write(100); wos.Write(200);
    wos.Close();

    ReadOnlyStream<int> ros(path, [](const string& s){ return stoi(s); });
    ros.Open();
    ASSERT(ros.Read() == 100, "File roundtrip: 100");
    ASSERT(ros.Read() == 200, "File roundtrip: 200");
    ros.Close();
}

// ================================================================
//  ТЕСТЫ вспомогательных функций
// ================================================================

void test_stream_utils() {
    cout << "\n-- StreamCopy / StreamFilter / StreamMap --\n";
    int arr[] = {1, 2, 3, 4, 5};

    // Copy
    MutableArraySequence<int> dst1;
    { MutableArraySequence<int> src(arr,5);
      ReadOnlyStream<int> r(&src); WriteOnlyStream<int> w(&dst1);
      r.Open(); w.Open(); StreamCopy(r,w); r.Close(); w.Close(); }
    ASSERT(dst1.GetLength()==5 && dst1.Get(4)==5, "StreamCopy");

    // Filter
    MutableArraySequence<int> dst2;
    { MutableArraySequence<int> src(arr,5);
      ReadOnlyStream<int> r(&src); WriteOnlyStream<int> w(&dst2);
      r.Open(); w.Open();
      StreamFilter<int>(r, w, [](const int& x){ return x%2==0; });
      r.Close(); w.Close(); }
    ASSERT(dst2.GetLength()==2 && dst2.Get(0)==2, "StreamFilter even");

    // Map
    MutableArraySequence<int> dst3;
    { MutableArraySequence<int> src(arr,5);
      ReadOnlyStream<int> r(&src); WriteOnlyStream<int> w(&dst3);
      r.Open(); w.Open();
      StreamMap<int>(r, w, [](const int& x){ return x*10; });
      r.Close(); w.Close(); }
    ASSERT(dst3.Get(0)==10 && dst3.Get(4)==50, "StreamMap *10");
}

void test_stream_not_open() {
    cout << "\n-- Stream: not-open guard --\n";
    int arr[] = {1};
    MutableArraySequence<int> seq(arr,1);
    ReadOnlyStream<int> ros(&seq);
    ASSERT_THROW(ros.Read(), "Read without Open throws");

    MutableArraySequence<int> dst;
    WriteOnlyStream<int> wos(&dst);
    ASSERT_THROW(wos.Write(1), "Write without Open throws");
}


void test_lazy_constructors_from_sequence() {
    cout << "\n-- LazySeq: constructors from Sequence* (spec 4a/4b) --\n";

    // 4a: указатель на функцию + Sequence* seeds
    int seedArr[] = {0, 1};
    MutableArraySequence<int> seeds_seq(seedArr, 2);

    auto fibFunc = [](const Sequence<int>* c) -> int {
        int n = c->GetLength();
        return c->Get(n-1) + c->Get(n-2);
    };
    // через указатель на функцию нельзя передать лямбду напрямую,
    // поэтому тестируем конструктор 4b (std::function + Sequence*)
    LazySequence<int> fib_b(
        std::function<int(const Sequence<int>*)>(fibFunc),
        (Sequence<int>*)&seeds_seq
    );
    ASSERT(fib_b.GetLength() == Cardinal::Omega(), "4b: infinite");
    ASSERT(fib_b.Get(0) == 0,  "4b: fib[0]==0");
    ASSERT(fib_b.Get(5) == 5,  "4b: fib[5]==5");
    ASSERT(fib_b.Get(10) == 55,"4b: fib[10]==55");

    // Проверяем что seeds скопированы корректно (не зависим от seeds_seq)
    // изменим seeds_seq после создания — LazySequence не должна измениться
    // (seeds уже в кэше, source=nullptr)
    ASSERT(fib_b.GetMaterializedCount() >= 2, "4b: seeds in cache");

    // 3: из Sequence* — ленивая загрузка конечной последовательности
    int arr[] = {10, 20, 30, 40, 50};
    MutableArraySequence<int> src(arr, 5);
    LazySequence<int> lazy_seq((Sequence<int>*)&src);
    ASSERT(lazy_seq.GetMaterializedCount() == 0,    "3: nothing materialized");
    ASSERT(lazy_seq.Get(2) == 30,                   "3: Get(2)==30");
    ASSERT(lazy_seq.GetMaterializedCount() == 3,    "3: 3 materialized");
    ASSERT(lazy_seq.GetLength() == Cardinal(5),     "3: length==5");
}


void test_lazy_subsequence_patches() {
    cout << "\n-- LazySeq: subsequence patches (InsertAt/Remove/Concat) --\n";

    // InsertAt(Sequence*, index) — вставка блока
    int arr[] = {1, 2, 3, 4, 5};
    LazySequence<int> ls(arr, 5);

    int ins[] = {10, 20, 30};
    MutableArraySequence<int> insSeq(ins, 3);
    ls.InsertAt((Sequence<int>*)&insSeq, 2); // [1,2,10,20,30,3,4,5]

    ASSERT(ls.GetLength() == Cardinal(8),  "InsertAt(seq): length==8");
    ASSERT(ls.Get(0) == 1,   "InsertAt(seq): Get(0)==1");
    ASSERT(ls.Get(2) == 10,  "InsertAt(seq): Get(2)==10");
    ASSERT(ls.Get(3) == 20,  "InsertAt(seq): Get(3)==20");
    ASSERT(ls.Get(4) == 30,  "InsertAt(seq): Get(4)==30");
    ASSERT(ls.Get(5) == 3,   "InsertAt(seq): Get(5)==3");

    // Remove(index, count) — удаление блока
    int arr2[] = {1, 2, 3, 4, 5};
    LazySequence<int> ls2(arr2, 5);
    ls2.Remove(1, 3); // удаляем [2,3,4] -> [1, 5]

    ASSERT(ls2.GetLength() == Cardinal(2), "Remove(1,3): length==2");
    ASSERT(ls2.Get(0) == 1, "Remove(1,3): Get(0)==1");
    ASSERT(ls2.Get(1) == 5, "Remove(1,3): Get(1)==5");
    ASSERT_THROW(ls2.Get(2), "Remove(1,3): Get(2) throws");

    // Ленивый Concat — один патч на всю подпоследовательность
    int a[] = {1, 2, 3};
    int b[] = {4, 5, 6};
    LazySequence<int> la(a, 3);
    LazySequence<int> lb(b, 3);

    ASSERT(la.GetMaterializedCount() == 3, "Before Concat: la materialized==3");
    la.Concat(&lb);
    ASSERT(la.GetLength() == Cardinal(6),  "Concat: length==6");
    // Ключевое: lb не материализован, la тоже не запрашивал lb
    ASSERT(lb.GetMaterializedCount() == 3, "Concat: lb not materialized extra");
    ASSERT(la.Get(3) == 4, "Concat: Get(3)==4");
    ASSERT(la.Get(5) == 6, "Concat: Get(5)==6");

    // Prepend(Sequence*) — вставка блока в начало
    int arr3[] = {10, 20};
    int arr4[] = {1, 2, 3};
    LazySequence<int> ls3(arr4, 3);
    MutableArraySequence<int> preSeq(arr3, 2);
    ls3.Prepend((Sequence<int>*)&preSeq); // [10,20,1,2,3]

    ASSERT(ls3.GetLength() == Cardinal(5), "Prepend(seq): length==5");
    ASSERT(ls3.Get(0) == 10, "Prepend(seq): Get(0)==10");
    ASSERT(ls3.Get(2) == 1,  "Prepend(seq): Get(2)==1");

    // Комбинация: InsertAt(seq) + Remove + InsertAt(single)
    int arr5[] = {1, 2, 3, 4, 5};
    LazySequence<int> ls4(arr5, 5);
    int extra[] = {10, 11};
    MutableArraySequence<int> extraSeq(extra, 2);
    ls4.InsertAt((Sequence<int>*)&extraSeq, 1); // [1,10,11,2,3,4,5]
    ls4.Remove(3, 2);                            // удаляем [2,3] -> [1,10,11,4,5]
    ls4.InsertAt(99, 2);                         // [1,10,99,11,4,5]

    ASSERT(ls4.GetLength() == Cardinal(6), "Combined patches: length==6");
    ASSERT(ls4.Get(0) == 1,  "Combined: Get(0)==1");
    ASSERT(ls4.Get(1) == 10, "Combined: Get(1)==10");
    ASSERT(ls4.Get(2) == 99, "Combined: Get(2)==99");
    ASSERT(ls4.Get(3) == 11, "Combined: Get(3)==11");
    ASSERT(ls4.Get(4) == 4,  "Combined: Get(4)==4");
    ASSERT(ls4.Get(5) == 5,  "Combined: Get(5)==5");
}


void test_lazy_insert_is_lazy() {
    cout << "\n-- LazySeq: InsertAt(LazySeq*) is truly lazy --\n";

    // Создаём бесконечную Fibonacci как вставляемую последовательность
    auto fibRule = [](const Sequence<int>* c) -> int {
        int n = c->GetLength();
        return c->Get(n-1) + c->Get(n-2);
    };
    int seeds[] = {0, 1};
    LazySequence<int>* fib = new LazySequence<int>(fibRule, seeds, 2);

    // Вставляем её в позицию 1 конечной последовательности [10, 20]
    int arr[] = {10, 20};
    LazySequence<int> ls(arr, 2);
    ls.InsertAt(*fib, 1);  // [10, fib[0], fib[1], ...] — бесконечная! (fib копируется)
    delete fib;

    // Читаем первые элементы — fib материализуется лениво по мере чтения
    ASSERT(ls.Get(0) == 10,  "ls[0] == 10");
    ASSERT(ls.Get(1) == 0,   "ls[1] == fib[0] == 0");
    ASSERT(ls.Get(2) == 1,   "ls[2] == fib[1] == 1");
    ASSERT(ls.Get(5) == 3,   "ls[5] == fib[4] == 3");
    // После std::move(*fib) объект fib перемещён — проверяем результат через ls
    ASSERT(ls.Get(5) == 3, "Fib: Get(5)==fib[4]==3 (lazy materialization)");

    // InsertAt(LazySequence*) с конечной последовательностью — тоже ленивый
    int arr2[] = {1, 2, 3, 4, 5};
    LazySequence<int> ls2(arr2, 5);
    LazySequence<int>* toInsert = new LazySequence<int>(arr2, 3); // [1,2,3]
    ASSERT(toInsert->GetMaterializedCount() == 3, "toInsert: from array, all in cache");
    ls2.InsertAt(*toInsert, 2); // [1,2,1,2,3,3,4,5] (копируется)
    delete toInsert;
    ASSERT(ls2.GetLength() == Cardinal(8), "InsertAt(lazy): length==8");
    ASSERT(ls2.Get(2) == 1, "InsertAt(lazy): Get(2)==1");
    ASSERT(ls2.Get(4) == 3, "InsertAt(lazy): Get(4)==3");
    ASSERT(ls2.Get(5) == 3, "InsertAt(lazy): Get(5)==3");
}


void test_lazy_count_bounds() {
    cout << "\n-- LazySeq: count as upper limit (min semantics) --\n";
    int arr[] = {1, 2, 3};
    LazySequence<int> ls(arr, 3);

    // count > len: не ошибка, просто берём min(count, len)
    auto m = ls.Map([](const int& x){ return x*2; }, 100);
    ASSERT(m->GetLength() == Cardinal(3), "Map count>len: clipped to len");
    ASSERT(m->Get(2) == 6, "Map count>len: correct values");
    delete m;

    auto w = ls.Where([](const int& x){ return x > 1; }, 100);
    ASSERT(w->GetLength() == Cardinal(2), "Where count>len: clipped");
    delete w;

    int s = ls.Reduce(0, [](const int& a, const int& b){ return a+b; }, 100);
    ASSERT(s == 6, "Reduce count>len: sum==6");

    // count = -1: берём всю длину
    auto m2 = ls.Map([](const int& x){ return x; }, IGenerator<int>::INF);
    ASSERT(m2->GetLength() == Cardinal(3), "Map count=INF: full length");
    delete m2;

    // Для бесконечной count обязателен
    auto fibRule = [](const Sequence<int>* c) -> int {
        int n = c->GetLength(); return c->Get(n-1) + c->Get(n-2);
    };
    int seeds[] = {0, 1};
    LazySequence<int> fib(fibRule, seeds, 2);
    ASSERT_THROW(fib.Map([](const int& x){ return x; }), "Infinite Map count=INF throws");
    auto mf = fib.Map([](const int& x){ return x; }, 10);
    ASSERT(mf->GetLength() == Cardinal(10), "Infinite Map count=10: ok");
    delete mf;
}


void test_ordinal() {
    cout << "\n-- Ordinal type --\n";
    Ordinal o0(5);
    ASSERT(o0.IsFinite(),          "Ordinal(5) is finite");
    ASSERT(o0.k == 0 && o0.n == 5, "Ordinal(5): k=0, n=5");

    Ordinal o1 = Ordinal::Omega();
    ASSERT(o1.IsInfinite(),        "Omega is infinite");
    ASSERT(o1.k == 1 && o1.n == 0, "Omega: k=1, n=0");

    Ordinal o2 = Ordinal::OmegaKPlusN(2, 10);
    ASSERT(o2.k == 2 && o2.n == 10, "omega*2+10: k=2, n=10");

    ASSERT(Ordinal(3) < Ordinal(5),          "3 < 5");
    ASSERT(Ordinal(5) < Ordinal::Omega(),    "5 < omega");
    ASSERT(Ordinal(1,0) < Ordinal(2,0),      "omega < omega*2");
    ASSERT(Ordinal(1,5) < Ordinal(2,0),      "omega+5 < omega*2");
    ASSERT(!(Ordinal(1,0) < Ordinal(1,0)),   "omega not < omega");
}

void test_concat_generator() {
    cout << "\n-- ConcatGenerator: Get(omega*k + n) --\n";

    // Три бесконечные последовательности
    auto fibRule = [](const Sequence<int>* c) -> int {
        int n = c->GetLength(); return c->Get(n-1) + c->Get(n-2);
    };
    int fibSeeds[] = {0, 1};
    LazySequence<int> fib(fibRule, fibSeeds, 2);  // 0,1,1,2,3,5,8...

    int apSeeds[] = {0};
    auto apRule = [](const Sequence<int>* c) -> int {
        return c->Get(c->GetLength()-1) + 2;
    };
    LazySequence<int> ap(apRule, apSeeds, 1);     // 0,2,4,6,8...

    int natSeeds[] = {0};
    auto natRule = [](const Sequence<int>* c) -> int {
        return c->Get(c->GetLength()-1) + 1;
    };
    LazySequence<int> nat(natRule, natSeeds, 1);  // 0,1,2,3,4...

    // Цепочка Concat — как в задании
    fib.Concat(&ap)->Concat(&nat);

    // Get(ω*0 + n) — из fib
    ASSERT(fib.Get(Ordinal(0, 0)) == 0,  "concat[omega*0+0] == fib[0] == 0");
    ASSERT(fib.Get(Ordinal(0, 5)) == 5,  "concat[omega*0+5] == fib[5] == 5");
    ASSERT(fib.Get(Ordinal(0,10)) == 55, "concat[omega*0+10] == fib[10] == 55");

    // Get(ω*1 + n) — из ap
    ASSERT(fib.Get(Ordinal(1, 0)) == 0,  "concat[omega*1+0] == ap[0] == 0");
    ASSERT(fib.Get(Ordinal(1, 3)) == 6,  "concat[omega*1+3] == ap[3] == 6");

    // Get(ω*2 + n) — из nat — «десятый элемент третьей последовательности»
    ASSERT(fib.Get(Ordinal(2, 0))  == 0,  "concat[omega*2+0] == nat[0] == 0");
    ASSERT(fib.Get(Ordinal(2, 10)) == 10, "concat[omega*2+10] == nat[10] == 10");

    // Обычный Get(int) на конечных всё ещё работает
    int arr[] = {10, 20, 30};
    LazySequence<int> ls(arr, 3);
    ASSERT(ls.Get(0) == 10, "finite Get(0) still works");
    ASSERT(ls.Get(2) == 30, "finite Get(2) still works");
    // Concat конечных — по-прежнему через патч
    LazySequence<int> ls2(arr, 3);
    int arr2[] = {40, 50};
    LazySequence<int> ext(arr2, 2);
    ls2.Concat(&ext);
    ASSERT(ls2.Get(3) == 40, "finite Concat: Get(3)==40");
    ASSERT(ls2.GetLength() == Cardinal(5), "finite Concat: length==5");
}


void test_insert_into_infinite() {
    cout << "\n-- InsertAt into infinite (RuleGenerator) --\n";

    // Фибоначчи: 0,1,1,2,3,5,8,13,21,...
    auto fibRule = [](const Sequence<int>* c) -> int {
        int n = c->GetLength(); return c->Get(n-1) + c->Get(n-2);
    };
    int seeds[] = {0, 1};
    LazySequence<int> fib(fibRule, seeds, 2);

    // Вставляем 17 на логическую позицию 7
    // До: [0,1,1,2,3,5,8,13,21,...]
    // После: [0,1,1,2,3,5,8, 17, 13,21,...]
    fib.InsertAt(17, 7);

    ASSERT(fib.Get(0) == 0,  "fib[0] == 0");
    ASSERT(fib.Get(5) == 5,  "fib[5] == 5");
    ASSERT(fib.Get(6) == 8,  "fib[6] == 8");
    ASSERT(fib.Get(7) == 17, "fib[7] == 17 (inserted)");
    // Правило работает по физическому кэшу — f(8)=13 независимо от вставки
    ASSERT(fib.Get(8) == 13, "fib[8] == 13 (rule unaffected by insert)");
    ASSERT(fib.Get(9) == 21, "fib[9] == 21");

    // Prepend в бесконечную
    LazySequence<int> fib2(fibRule, seeds, 2);
    fib2.Prepend(99);
    ASSERT(fib2.Get(0) == 99, "After Prepend: fib2[0] == 99");
    ASSERT(fib2.Get(1) == 0,  "After Prepend: fib2[1] == fib[0] == 0");
    ASSERT(fib2.Get(3) == 1,  "After Prepend: fib2[3] == fib[2] == 1");
}


void test_patches_on_infinite() {
    cout << "\n-- Patches on infinite (RuleGenerator) --\n";

    auto fibRule = [](const Sequence<int>* c) -> int {
        int n = c->GetLength(); return c->Get(n-1) + c->Get(n-2);
    };
    int seeds[] = {0, 1};

    // ---- InsertAt: одиночный элемент ----
    {
        LazySequence<int> fib(fibRule, seeds, 2);
        fib.InsertAt(17, 7);
        // [0,1,1,2,3,5,8, 17, 13,21,...]
        ASSERT(fib.Get(6) == 8,  "InsertAt single: Get(6)==8");
        ASSERT(fib.Get(7) == 17, "InsertAt single: Get(7)==17");
        ASSERT(fib.Get(8) == 13, "InsertAt single: Get(8)==13 (rule unaffected)");
        ASSERT(fib.Get(9) == 21, "InsertAt single: Get(9)==21");
    }

    // ---- InsertAt: подпоследовательность ----
    {
        LazySequence<int> fib(fibRule, seeds, 2);
        int ins[] = {100, 200, 300};
        MutableArraySequence<int> insSeq(ins, 3);
        fib.InsertAt((Sequence<int>*)&insSeq, 2);
        // [0,1, 100,200,300, 1,2,3,5,8,...]
        ASSERT(fib.Get(2) == 100, "InsertAt seq: Get(2)==100");
        ASSERT(fib.Get(4) == 300, "InsertAt seq: Get(4)==300");
        ASSERT(fib.Get(5) == 1,   "InsertAt seq: Get(5)==fib[2]==1");
        ASSERT(fib.Get(7) == 3,   "InsertAt seq: Get(7)==fib[4]==3");
    }

    // ---- Prepend ----
    {
        LazySequence<int> fib(fibRule, seeds, 2);
        fib.Prepend(99);
        // [99, 0,1,1,2,3,5,8,...]
        ASSERT(fib.Get(0) == 99, "Prepend: Get(0)==99");
        ASSERT(fib.Get(1) == 0,  "Prepend: Get(1)==fib[0]==0");
        ASSERT(fib.Get(3) == 1,  "Prepend: Get(3)==fib[2]==1");
        ASSERT(fib.Get(8) == 13, "Prepend: Get(8)==fib[7]==13");
    }

    // ---- Append: запрещён для бесконечных ----
    {
        LazySequence<int> fib(fibRule, seeds, 2);
        ASSERT_THROW(fib.Append(42), "Append to infinite throws");
    }

    // ---- Remove одного элемента ----
    {
        LazySequence<int> fib(fibRule, seeds, 2);
        fib.RemoveAt(0);
        // [1,1,2,3,5,8,13,...]  (убрали fib[0]=0)
        ASSERT(fib.Get(0) == 1,  "RemoveAt(0): Get(0)==1");
        ASSERT(fib.Get(1) == 1,  "RemoveAt(0): Get(1)==1");
        ASSERT(fib.Get(5) == 8,  "RemoveAt(0): Get(5)==8");
    }

    // ---- Remove диапазона ----
    {
        LazySequence<int> fib(fibRule, seeds, 2);
        fib.Remove(1, 3);
        // [0, 3,5,8,13,...] (убрали fib[1]=1, fib[2]=1, fib[3]=2)
        ASSERT(fib.Get(0) == 0, "Remove(1,3): Get(0)==0");
        ASSERT(fib.Get(1) == 3, "Remove(1,3): Get(1)==fib[4]==3");
        ASSERT(fib.Get(2) == 5, "Remove(1,3): Get(2)==fib[5]==5");
    }

    // ---- Несколько патчей подряд ----
    {
        LazySequence<int> fib(fibRule, seeds, 2);
        fib.Prepend(99);    // [99, 0,1,1,2,3,5,8,...]
        fib.InsertAt(42, 3); // [99, 0,1, 42, 1,2,3,5,8,...]
        fib.RemoveAt(0);     // [0,1, 42, 1,2,3,5,8,...]
        ASSERT(fib.Get(0) == 0,  "Multi-patch: Get(0)==0");
        ASSERT(fib.Get(1) == 1,  "Multi-patch: Get(1)==1");
        ASSERT(fib.Get(2) == 42, "Multi-patch: Get(2)==42");
        ASSERT(fib.Get(3) == 1,  "Multi-patch: Get(3)==fib[2]==1");
        ASSERT(fib.Get(6) == 5,  "Multi-patch: Get(6)==fib[5]==5");
    }

    // ---- Map на бесконечной с патчем ----
    {
        LazySequence<int> fib(fibRule, seeds, 2);
        fib.InsertAt(17, 7);
        auto m = fib.Map([](const int& x){ return x * 2; }, 10);
        // [0,2,2,4,6,10,16, 34, 26,42]
        ASSERT(m->Get(7) == 34, "Map after InsertAt: Get(7)==34");
        ASSERT(m->Get(8) == 26, "Map after InsertAt: Get(8)==26");
        delete m;
    }
}


void test_transfinite_subsequence() {
    cout << "\n-- GetSubsequence(Ordinal, Ordinal) --\n";

    // fib:  0,1,1,2,3,5,8,13,21,34,55,...
    // nat:  0,1,2,3,4,5,6,7,8,9,10,...
    // even: 0,2,4,6,8,10,...
    auto fibRule = [](const Sequence<int>* c) -> int {
        int n = c->GetLength(); return c->Get(n-1) + c->Get(n-2);
    };
    auto natRule = [](const Sequence<int>* c) -> int {
        return c->Get(c->GetLength()-1) + 1;
    };
    auto evenRule = [](const Sequence<int>* c) -> int {
        return c->Get(c->GetLength()-1) + 2;
    };

    int fseeds[] = {0, 1};
    int nseeds[] = {0};
    int eseeds[] = {0};

    LazySequence<int> fib(fibRule, fseeds, 2);
    LazySequence<int> nat(natRule, nseeds, 1);
    LazySequence<int> even(evenRule, eseeds, 1);

    // Цепочка Concat — унифицированный интерфейс
    fib.Concat(&nat)->Concat(&even);
    // fib теперь содержит ConcatGenerator: fib | nat | even

    // ---- Один блок: от ω·0+10 до ω·0+12 → fib[10..12] = [55,89,144] ----
    {
        LazySequence<int>* sub1 = fib.GetSubsequence(Ordinal(0,10), Ordinal(0,12));
        ASSERT(sub1->Get(0) == 55,  "SubSeq same block: Get(0)==fib[10]==55");
        ASSERT(sub1->Get(1) == 89,  "SubSeq same block: Get(1)==fib[11]==89");
        ASSERT(sub1->Get(2) == 144, "SubSeq same block: Get(2)==fib[12]==144");
        ASSERT(sub1->GetLength() == Cardinal(3), "SubSeq same block: length==3");
        delete sub1;
    }

    // ---- Два блока: от ω·0+10 до ω·1+9
    //   fib[10..inf] сначала, потом nat[0..9]
    //   результат: [55,89,144,...] + [0,1,2,...,9] — бесконечный первый блок!
    {
        LazySequence<int>* sub2 = fib.GetSubsequence(Ordinal(0,10), Ordinal(1,9));
        // Первые элементы — хвост фибоначчи начиная с fib[10]
        ASSERT(sub2->Get(Ordinal(0,0)) == 55,  "SubSeq 2 blocks: Get(ω·0+0)==fib[10]==55");
        ASSERT(sub2->Get(Ordinal(0,3)) == 233, "SubSeq 2 blocks: Get(ω·0+3)==fib[13]==233");
        // Второй блок — nat[0..9]
        ASSERT(sub2->Get(Ordinal(1,0)) == 0,   "SubSeq 2 blocks: Get(ω·1+0)==nat[0]==0");
        ASSERT(sub2->Get(Ordinal(1,9)) == 9,   "SubSeq 2 blocks: Get(ω·1+9)==nat[9]==9");
        delete sub2;
    }

    // ---- Три блока: от ω·0+5 до ω·2+4
    //   fib[5..inf], nat[0..inf], even[0..4]
    {
        LazySequence<int>* sub3 = fib.GetSubsequence(Ordinal(0,5), Ordinal(2,4));
        ASSERT(sub3->Get(Ordinal(0,0)) == 5,   "SubSeq 3 blocks: Get(ω·0+0)==fib[5]==5");
        ASSERT(sub3->Get(Ordinal(0,2)) == 13,  "SubSeq 3 blocks: Get(ω·0+2)==fib[7]==13");
        ASSERT(sub3->Get(Ordinal(1,0)) == 0,   "SubSeq 3 blocks: Get(ω·1+0)==nat[0]==0");
        ASSERT(sub3->Get(Ordinal(1,5)) == 5,   "SubSeq 3 blocks: Get(ω·1+5)==nat[5]==5");
        ASSERT(sub3->Get(Ordinal(2,0)) == 0,   "SubSeq 3 blocks: Get(ω·2+0)==even[0]==0");
        ASSERT(sub3->Get(Ordinal(2,4)) == 8,   "SubSeq 3 blocks: Get(ω·2+4)==even[4]==8");
        delete sub3;
    }
}

void runAllTests() {
    testsPassed = 0; testsFailed = 0;
    cout << "========== UNIT TESTS ==========\n";
    test_lazy_finite_from_array();
    test_lazy_infinite_fibonacci();
    test_lazy_mutations();
    test_lazy_functional();
    test_lazy_constructors_from_sequence();
    test_lazy_subsequence_patches();
    test_lazy_insert_is_lazy();
    test_lazy_count_bounds();
    test_ordinal();
    test_concat_generator();
    test_insert_into_infinite();
    test_patches_on_infinite();
    test_transfinite_subsequence();
    test_ros_from_sequence();
    test_ros_seek();
    test_ros_goback();
    test_ros_from_lazy();
    test_ros_from_file();
    test_wos_to_sequence();
    test_wos_to_file();
    test_stream_utils();
    test_stream_not_open();
    cout << "\n========== RESULTS ==========\n";
    cout << "Passed: " << testsPassed << "\n";
    cout << "Failed: " << testsFailed << "\n";
}

// ================================================================
//  UI
// ================================================================

void uiReadOnlyStream() {
    cout << "\n=== ReadOnlyStream ===\n";
    cout << "1. Из массива  2. Из файла\nВыбор: ";
    int src; cin >> src;

    if (src == 1) {
        cout << "N и N чисел: ";
        int n; cin >> n;
        MutableArraySequence<int> seq;
        for (int i = 0; i < n; i++) { int x; cin >> x; seq.Append(x); }
        ReadOnlyStream<int> ros(&seq);
        ros.Open();
        int cmd;
        while (true) {
            cout << "\n1.Read  2.Seek(i)  3.GoBack  4.Position  5.IsEOS  0.Назад\nВыбор: ";
            cin >> cmd;
            if (cmd == 0) break;
            try {
                if      (cmd==1) cout << "Read = " << ros.Read() << "\n";
                else if (cmd==2) { size_t i; cout<<"i="; cin>>i; cout<<"Seek->"<<ros.Seek(i)<<"\n"; }
                else if (cmd==3) { ros.GoBack(); cout<<"pos="<<ros.GetPosition()<<"\n"; }
                else if (cmd==4) cout<<"Position="<<ros.GetPosition()<<"\n";
                else if (cmd==5) cout<<"IsEOS="<<(ros.IsEndOfStream()?"true":"false")<<"\n";
            } catch (exception& e) { cout << "Ошибка: " << e.what() << "\n"; }
        }
        ros.Close();
    } else {
        cout << "Путь к файлу: ";
        string path; cin >> path;
        ReadOnlyStream<int> ros(path, [](const string& s){ return stoi(s); });
        try { ros.Open(); } catch (exception& e) { cout << "Ошибка: " << e.what() << "\n"; return; }
        cout << "Содержимое: ";
        while (!ros.IsEndOfStream()) { try { cout << ros.Read() << " "; } catch (...) { break; } }
        cout << "\nПрочитано: " << ros.GetPosition() << "\n";
        ros.Close();
    }
}

void uiWriteOnlyStream() {
    cout << "\n=== WriteOnlyStream ===\n";
    cout << "1. В последовательность  2. В файл\nВыбор: ";
    int dst; cin >> dst;

    if (dst == 1) {
        MutableArraySequence<int> seq;
        WriteOnlyStream<int> wos(&seq);
        wos.Open();
        cout << "Числа (0 — завершить): ";
        int x; while (cin >> x && x != 0) wos.Write(x);
        wos.Close();
        cout << "Записано " << seq.GetLength() << " эл.: ";
        for (int i = 0; i < seq.GetLength(); i++) cout << seq.Get(i) << " ";
        cout << "\n";
    } else {
        cout << "Путь к файлу: ";
        string path; cin >> path;
        { ofstream f(path, ios::trunc); }
        WriteOnlyStream<int> wos(path, [](const int& x){ return to_string(x); });
        wos.Open();
        cout << "Числа (0 — завершить): ";
        int x; while (cin >> x && x != 0) wos.Write(x);
        wos.Close();
        cout << "Записано в: " << path << "\n";
    }
}

void uiLazyInfinite() {
    cout << "\n=== Бесконечная LazySequence ===\n";
    cout << "1. Fibonacci  2. Факториалы  3. Арифм. прогрессия\nВыбор: ";
    int c; cin >> c;

    LazySequence<long long>* ls = nullptr;
    if (c == 1) {
        auto r = [](const Sequence<long long>* ca) -> long long {
            int n=ca->GetLength(); return ca->Get(n-1)+ca->Get(n-2); };
        long long s[]={0,1}; ls = new LazySequence<long long>(r,s,2);
    } else if (c == 2) {
        auto r = [](const Sequence<long long>* ca) -> long long {
            int n=ca->GetLength(); return ca->Get(n-1)*(long long)n; };
        long long s[]={1}; ls = new LazySequence<long long>(r,s,1);
    } else {
        long long a0,d; cout<<"Первый: "; cin>>a0; cout<<"Разность: "; cin>>d;
        auto r = [d](const Sequence<long long>* ca) -> long long {
            return ca->Get(ca->GetLength()-1)+d; };
        long long s[]={a0}; ls = new LazySequence<long long>(r,s,1);
    }

    int cmd;
    while (true) {
        cout << "\n1.Get(i)  2.Вывести N  3.Map(*2,N)  4.Reduce(sum,N)  5.Поток из него  0.Назад\nВыбор: ";
        cin >> cmd;
        if (cmd == 0) break;
        try {
            if (cmd==1) { int i; cout<<"i="; cin>>i; cout<<"="<<ls->Get(i)<<"\n"; }
            else if (cmd==2) {
                int n; cout<<"N="; cin>>n;
                for (int i=0;i<n;i++) cout<<ls->Get(i)<<" "; cout<<"\n";
            }
            else if (cmd==3) {
                int n; cout<<"N="; cin>>n;
                auto m=ls->Map([](const long long& x){return x*2;},n);
                for (int i=0;i<n;i++) cout<<m->Get(i)<<" "; cout<<"\n"; delete m;
            }
            else if (cmd==4) {
                int n; cout<<"N="; cin>>n;
                long long s=ls->Reduce(0LL,[](const long long& a,const long long& b){return a+b;},n);
                cout<<"Sum="<<s<<"\n";
            }
            else if (cmd==5) {
                // Демо: читаем первые N через ReadOnlyStream
                int n; cout<<"N="; cin>>n;
                ReadOnlyStream<long long> ros(ls);
                ros.Open();
                cout<<"Через поток: ";
                for (int i=0;i<n && !ros.IsEndOfStream();i++) cout<<ros.Read()<<" ";
                cout<<"\n";
                ros.Close();
            }
        } catch (exception& e) { cout<<"Ошибка: "<<e.what()<<"\n"; }
    }
    delete ls;
}

int main() {
    int choice;
    cout << "===== Лабораторная работа №4 (LazySequence + Stream) =====\n";
    while (true) {
        cout << "\n1. Тесты\n2. ReadOnlyStream\n3. WriteOnlyStream\n4. Бесконечная LazySequence\n0. Выход\nВыбор: ";
        cin >> choice;
        if (choice == 0) { cout << "До свидания!\n"; break; }
        else if (choice == 1) runAllTests();
        else if (choice == 2) uiReadOnlyStream();
        else if (choice == 3) uiWriteOnlyStream();
        else if (choice == 4) uiLazyInfinite();
        else cout << "Неверный выбор.\n";
    }
    return 0;
}
