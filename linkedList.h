#ifndef LINKED_LIST_H
#define LINKED_LIST_H


#include <stdexcept>

template <class T> class LinkedList{
private:
    struct Node {
        T value;
        Node* next;
        
        Node(T val) : value(val), next(nullptr) {}
    };

    Node* head;
    Node* tail;
    int length;

public:
    LinkedList (const T* items, int count) : head(nullptr), tail(head), length(0){
        for (int i=0; i<count; i++){
            Append(items[i]);
        }
    } 
    LinkedList() : head(nullptr), tail(head), length(0) {}
    LinkedList (const LinkedList <T> & list) : head(nullptr), tail(head), length(0){
        for (int i=0; i<list.GetLength(); i++){
            Append(list.Get(i));
        }
    }

    ~LinkedList(){
        Node* current = head;
        while (current){
            Node* del = current;
            current = current->next;
            delete del;
        }
    }

    typedef Node* ListPosition;

    T GetFirst() const{
        if (!head){
            throw std::out_of_range("EmptyList");
        }
        return head->value;
    }

    ListPosition GetFirstPos() const{
        return head;
    }

    static ListPosition GetNextPos(ListPosition cur){
        return cur->next;
    }

    //static bool IsNotLastPos(ListPosition cur){
        //return cur->next;
        //return cur != nullptr && cur->next != nullptr;
    //}

    T GetLast() const{
        if (!head){
            throw std::out_of_range("EmptyList");
        }
        return tail->value;
    }

    static T Get(ListPosition pos){
        if (!pos){
            throw std::out_of_range("InvalidPosition");
        }
        return pos->value;
    }

    T Get(int index) const{
        if (!head){
            throw std::out_of_range("EmptyList");
        }
        if (index<0 || index>=length){
            throw std::out_of_range("IndexOutOfRange");
        }
        if(index==length-1){
            return tail->value;
        }
        Node* current = head;
        for (int i=0; i<index; i++){
            current = current->next;
        }
        return current->value;
    }

    LinkedList<T>* GetSubList(int startIndex, int endIndex) const{
        if (startIndex<0 || endIndex<0 || startIndex>=length || endIndex>=length || startIndex>endIndex){
            throw std::out_of_range("IndexOutOfRange");
        }
        LinkedList<T>* result = new LinkedList<T>;
        Node* current = head;
        for (int i=0; i<startIndex;i++){
            current = current->next;
        }
        
        for (int i=startIndex; i<=endIndex;i++){
            result->Append(current->value);
            current = current->next;
        }

        return result;
    } 

    int GetLength() const{
        return length;
    }

    void Append(const T& item){
        Node* appended = new Node(item);
        if (!head){
            head = appended;
        }
        else{
            tail->next = appended;
        }
        tail = appended;
        length++;
    }

    void Prepend(const T& item){
        Node* prepended = new Node(item);
        if (!head){
            tail = prepended;
        }
        prepended->next = head;
        head = prepended;
        length++;
    }

    void InsertAt(const T& item, int index){
        if (index<0 || index>length){
            throw std::out_of_range("IndexOutOfRange");
        }
        if (index==0){
            Prepend(item);
            return;
        }
        if (index==length){
            Append(item);
            return;
        }
        Node* inserted = new Node(item);
        Node* current = head;
        for (int i=0; i<index-1;i++){
            current=current->next;
        }
        inserted->next = current->next;
        current->next = inserted;
        length++;
        
    }

    void RemoveFirst() {
        if (!head) {
            throw std::out_of_range("EmptyList");
        }

        Node* temp = head;
        head = head->next;
        delete temp;

        if (length == 0) {
            tail = head;
        }

        length--;
    }


    void RemoveLast() {
        if (length == 0) {
            throw std::out_of_range("EmptyList");
        }

        if (length == 1) {
            delete head;
            head = nullptr;
            tail = nullptr;
        } else {
            Node* current = head;
            while (current->next != tail) {
                current = current->next;
            }

            delete tail;
            tail = current;
            tail->next = nullptr;
        }

        length--;
    }

};

#endif