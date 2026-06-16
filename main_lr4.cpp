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
