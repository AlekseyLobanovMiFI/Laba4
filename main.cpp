#include <iostream>
#include <stdexcept>

#include "stack.h"
#include "queue.h"
#include "deque.h"

using namespace std;

// ===== PRINT FUNCTIONS =====
template<class T>
void PrintStack(const Stack<T>& st) {
    for (int i = 0; i < st.Size(); i++) {
        cout << st.Get(i) << " ";
    }
    cout << endl;
}

template<class T>
void PrintQueue(const Queue<T>& q) {
    for (int i = 0; i < q.Size(); i++) {
        cout << q.Get(i) << " ";
    }
    cout << endl;
}

template<class T>
void PrintDeque(const Deque<T>& dq) {
    for (int i = 0; i < dq.Size(); i++) {
        cout << dq.Get(i) << " ";
    }
    cout << endl;
}

// ================= STACK =================
void StackUI() {
    Stack<int> st;
    int cmd;

    while (true) {
        cout << "\n--- STACK ---\n";
        cout << "1. Push\n2. Pop\n3. Top\n4. Print\n5. Clear\n0. Back\nChoice: ";
        cin >> cmd;

        if (cmd == 0) break;

        try {
            if (cmd == 1) {
                int x;
                cout << "Enter value: ";
                cin >> x;
                st.Push(x);
            }
            else if (cmd == 2) {
                cout << "Popped: " << st.Pop() << endl;
            }
            else if (cmd == 3) {
                cout << "Top: " << st.Top() << endl;
            }
            else if (cmd == 4) {
                PrintStack(st);
            }
            else if (cmd == 5) {
                st.Clear();
                cout << "Stack cleared\n";
            }
        } catch (exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    }
}

// ================= QUEUE =================
void QueueUI() {
    Queue<int> q;
    int cmd;

    while (true) {
        cout << "\n--- QUEUE ---\n";
        cout << "1. Enqueue\n2. Dequeue\n3. Front\n4. Print\n5. Clear\n0. Back\nChoice: ";
        cin >> cmd;

        if (cmd == 0) break;

        try {
            if (cmd == 1) {
                int x;
                cout << "Enter value: ";
                cin >> x;
                q.Enqueue(x);
            }
            else if (cmd == 2) {
                cout << "Dequeued: " << q.Dequeue() << endl;
            }
            else if (cmd == 3) {
                cout << "Front: " << q.Front() << endl;
            }
            else if (cmd == 4) {
                PrintQueue(q);
            }
            else if (cmd == 5) {
                q.Clear();
                cout << "Queue cleared\n";
            }
        } catch (exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    }
}

// ================= DEQUE =================
void DequeUI() {
    Deque<int> dq;
    int cmd;

    while (true) {
        cout << "\n--- DEQUE ---\n";
        cout << "1. PushFront\n2. PushBack\n3. PopFront\n4. PopBack\n5. Front\n6. Back\n7. Print\n8. Clear\n0. Back\nChoice: ";
        cin >> cmd;

        if (cmd == 0) break;

        try {
            if (cmd == 1) {
                int x;
                cout << "Enter value: ";
                cin >> x;
                dq.PushFront(x);
            }
            else if (cmd == 2) {
                int x;
                cout << "Enter value: ";
                cin >> x;
                dq.PushBack(x);
            }
            else if (cmd == 3) {
                cout << "PopFront: " << dq.PopFront() << endl;
            }
            else if (cmd == 4) {
                cout << "PopBack: " << dq.PopBack() << endl;
            }
            else if (cmd == 5) {
                cout << "Front: " << dq.Front() << endl;
            }
            else if (cmd == 6) {
                cout << "Back: " << dq.Back() << endl;
            }
            else if (cmd == 7) {
                PrintDeque(dq);
            }
            else if (cmd == 8) {
                dq.Clear();
                cout << "Deque cleared\n";
            }
        } catch (exception& e) {
            cout << "Error: " << e.what() << endl;
        }
    }
}

// ================= MAIN =================
int main() {
    int choice;

    cout << "START\n";

    while (true) {
        cout << "\n===== MAIN MENU =====\n";
        cout << "1. Stack\n2. Queue\n3. Deque\n0. Exit\nChoice: ";
        cin >> choice;

        if (choice == 0) {
            cout << "Bye!\n";
            break;
        }

        if (choice == 1) {
            StackUI();
        }
        else if (choice == 2) {
            QueueUI();
        }
        else if (choice == 3) {
            DequeUI();
        }
        else {
            cout << "Invalid choice\n";
        }
    }

    return 0;
}