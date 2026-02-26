#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>


/**
 * Ported a reduced version of LinkedBlockingQueue from Java over to C++.
 * @tparam E
 */
template <typename E>
class LinkedBlockingQueue {
private:
    class Node {
        private:
            Node *next;
            E item;
        public:
            Node() : next(nullptr) {}

            Node(const E &item) : item(item), next(nullptr) {}

            ~Node() = default;

            bool hasNext() {
                return next != nullptr;
            }

            void setNext(Node *nextOne) {
                next = nextOne;
            }
            Node* getNext() {
                return next;
            }
            E getItem() {
                return item;
            }
        };
    Node *head;
    Node *tail;

    std::atomic<int> queue_size;
    int max_queue_size;

    std::mutex mutexOne;
    std::mutex mutexTwo;

    std::condition_variable cond1;
    std::condition_variable cond2;


    public:
    LinkedBlockingQueue() {
        this->head = new Node();
        this->tail = nullptr;
        this->queue_size = 0;
        this->max_queue_size = 100;
    }
    ~LinkedBlockingQueue();

    bool offer(E &item);
    E getListItem();

};

template <typename E>
LinkedBlockingQueue<E>::~LinkedBlockingQueue() {
    Node *current = head;
    while (current != nullptr) {
        Node* next = current->getNext();
        delete current;
        current = next;
    }
}

template <typename E>
bool LinkedBlockingQueue<E>::offer(E &item) {
    std::unique_lock<std::mutex> lock(mutexTwo);

    if (queue_size == max_queue_size ) {
        lockTwo.unlock();
        return false;
    }

    Node *newNode = new Node(item);

    if (tail == nullptr) {
        tail = newNode;
        head->setNext(newNode);
    }

    else {
        tail->setNext(newNode);
        tail = newNode;
    }
    ++queue_size;

    return true;
}


template <typename E>
E LinkedBlockingQueue<E>::getListItem() {
    std::unique_lock<std::mutex> lock(mutexOne);

    if (head->getNext() == nullptr) {
        cond1.wait(lock, [this]{return head->hasNext();});
    }
    Node *current = head->getNext();
    head->setNext(current->getNext());

    E result = current->getItem();

    if (tail == current) {
        tail = nullptr;
    }

    delete current;
    --queue_size;
    return &result;

}