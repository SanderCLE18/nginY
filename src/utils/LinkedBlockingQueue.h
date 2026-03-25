#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

/**
 * Ported a reduced version of LinkedBlockingQueue from Java over to C++.
 * @tparam E
 */
template<typename E>
class LinkedBlockingQueue {
private:
    /**
     * @brief Internal node class used for storing tasks.
     */
    class Node {
    private:
        /**
         * @brief a pointer to the next node in the queue
         */
        Node *next;
        /**
         * @brief the task item stored in the node
         */
        E item;

    public:
        /**
         * @brief Constructor for the Node class.
         * Sets the next node as a nullptr.
         * Allows for the creation of dummy nodes.
         */
        Node() : next(nullptr) {
        }

        /**
         * @brief Constructor for Node class, sets the next node as a nullptr and takes in an item
         * @param item the item to be stored in the node
         */
        Node(const E &item) : item(item), next(nullptr) {
        }

        /**
         * @brief Destructor for Node class, cleans up any resources held by the node
         */
        ~Node() = default;

        /**
         * @brief Function for checking if the next Node is not null.
         *
         * @return True if the next Node is not null, otherwise false.
         */
        bool hasNext() {
            return next != nullptr;
        }

        /**
         * @brief Function for setting the next Node in the linked list
         *
         * @param nextOne The next node in the linked list
         */
        void setNext(Node *nextOne) {
            next = nextOne;
        }

        /**
         * @brief Function for getting a pointer to the next node in the list.
         *
         * @return Returns a pointer to the next node in the linked list
         */
        Node *getNext() {
            return next;
        }

        /**
         * @brief Function for getting the stored item
         *
         * @return Stored item
         */
        E getItem() {
            return item;
        }
    };

    /**
     * @brief Dummy node that acts as a placeholder for the head of the linked list.
     * Will never be deleted or changed.
     */
    Node *head;
    /**
     * @brief Dummy node that acts as a placeholder for the tail of the linked list.
     * Will never be deleted or changed.
     */
    Node *tail;

    /**
     * @brief Atomic boolean flag indicating whether the queue is in the process of shutting down.
     */
    std::atomic<bool> shutdown;

    /**
     * @brief Atomic integer representing the current size of the queue.
     */
    std::atomic<int> queueSize;

    /**
     * @brief Maximum size of the queue.
     */
    int maxQueueSize;

    /**
     * @brief Mutex for synchronization during offer and put operations.
     */
    std::mutex mutexOne;
    /**
     * @brief Mutex for synchronization during get operations.
     */
    std::mutex mutexTwo;

    /**
     * @brief Condition variable for signaling when the queue is not empty.
     */
    std::condition_variable notEmpty;

    /**
     * @brief Condition variable for signaling when the queue is not full.
     */
    std::condition_variable notFull;

public:
    /**
     * @brief Constructor for LinkedBlockingQueue.
     */
    LinkedBlockingQueue() {
        this->head = new Node();
        this->tail = head;
        this->queueSize = 0;
        this->maxQueueSize = 100;
        this->shutdown = false;
    }

    /**
     * @brief Destructor for LinkedBlockingQueue.
     */
    ~LinkedBlockingQueue();

    /**
     * @brief Function for offering items to the queue.
     *
     * @param item item offered for the queue
     * @return true if successful, otherwise false.
     */
    bool offer(const E &item);

    /**
     * @brief Function for adding items to the queue.
     * Waits until it is possible to add the item to the queue.
     *
     * @param item Item to be added to the queue.
     */
    void put(const E &item);

    /**
     * @brief Function for retrieving items from the queue.
     * Will wait until it is possible to retrieve the item from the queue.
     * @return retrieved item or std::nullopt if queue is empty
     */
    std::optional<E> getListItem();

    /**
     * @brief Function for retriving items from the queue.
     *
     * @return retrieved item or std::nullopt if queue is empty
     */
    std::optional<E> tryGetListItem();

    /**
     * @brief Shuts down the LinkedBlockingQueue
     */
    void cleanup();
};

template<typename E>
LinkedBlockingQueue<E>::~LinkedBlockingQueue() {
    Node *current = head;
    while (current != nullptr) {
        Node *next = current->getNext();
        delete current;
        current = next;
    }
}

template<typename E>
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

template<typename E>
void LinkedBlockingQueue<E>::put(const E &item) {
    Node *newNode = new Node(item);

    {
        std::unique_lock<std::mutex> lock(mutexTwo);
        notFull.wait(lock, [this] { return queueSize <= maxQueueSize || shutdown; });
        if (shutdown) return;
        tail->setNext(newNode);
        tail = newNode;
        ++queueSize;
    }
    notEmpty.notify_one();
}

template<typename E>
std::optional<E> LinkedBlockingQueue<E>::getListItem() {
    std::optional<E> result;
    {
        std::unique_lock<std::mutex> lock(mutexOne);
        notEmpty.wait(lock, [this] { return head->hasNext() || shutdown; });
        if (shutdown) return std::nullopt;

        Node *current = head->getNext();
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
std::optional<E> LinkedBlockingQueue<E>::tryGetListItem() {
    std::optional<E> result;
    {
        std::unique_lock<std::mutex> lock(mutexOne);
        if (!head->hasNext()) {
            return std::nullopt;
        }
        Node *current = head->getNext();
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
        return result;
    }
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
