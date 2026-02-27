#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

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

    std::atomic<bool>shutdown;
    std::atomic<int> queueSize;
    int maxQueueSize;

    std::mutex mutexOne;
    std::mutex mutexTwo;

    std::mutex mutexStop;

    std::condition_variable notEmpty;
    std::condition_variable notFull;


    public:
    LinkedBlockingQueue() {
        this->head = new Node();
        this->tail = head;
        this->queueSize = 0;
        this->maxQueueSize = 100;
        this->shutdown = false;
    }
    ~LinkedBlockingQueue();

    bool offer(const E &item);
    void put(const E &item);
    std::optional<E> getListItem();
    void cleanup();
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
bool LinkedBlockingQueue<E>::offer(const E &item) {

    if (queueSize == maxQueueSize) {
        return false;
    }

    Node *newNode = new Node(item);

    {
        std::unique_lock<std::mutex> lock(mutexOne);
        tail->setNext(newNode);
        tail = newNode;
        ++queueSize;
    }

    notEmpty.notify_one();
    return true;
}

template <typename E>
void LinkedBlockingQueue<E>::put(const E &item) {
    Node *newNode = new Node(item);

    {
        std::unique_lock<std::mutex> lock(mutexTwo);
        notFull.wait(lock, [this]{return queueSize <= maxQueueSize || shutdown;});
        if (shutdown) return;
        tail->setNext(newNode);
        tail = newNode;
        ++queueSize;
    }
    notEmpty.notify_one();
}

template <typename E>
std::optional<E> LinkedBlockingQueue<E>::getListItem() {
    std::optional<E> result;
    {
        std::unique_lock<std::mutex> lock(mutexOne);
        notEmpty.wait(lock, [this]{return head->hasNext() || shutdown;});
        if (shutdown) return std::nullopt;

        Node * current = head->getNext();
        head->setNext(current->getNext());

        if (current->getNext() == nullptr) {
            std::unique_lock<std::mutex> putLock(mutexTwo);
            if (tail == current) {
                tail = head;
            }
        }
        result = current->getItem();
        delete current;
        --queueSize;

    }
    notFull.notify_one();
    return result;

}

template<typename E>
void LinkedBlockingQueue<E>::cleanup() {
    {
        std::unique_lock<std::mutex> lock(mutexOne);
        shutdown = true;
    }

    notFull.notify_all();
    notEmpty.notify_all();
}
