//
// Created by Sande on 20.03.2026.
//
#include <gtest/gtest.h>
#include "core/Threadpool.h"


TEST(ThreadPoolTest, SubmitReturnsCorrectFuture) {
    ThreadPool pool(4);
    std::future<int> future = pool.submit([](int x, int y)
        { return x + y; },
        21, 21);

    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTest, ConcurrentTasksExecute) {
    ThreadPool pool(8);
    constexpr int N = 10000;
    std::atomic<int> counter(0);

    std::vector<std::future<void>> futures;
    futures.reserve(N);

    for (int i = 0; i < N; i++) {
        futures.push_back(pool.submit([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }
    for (auto& f : futures) {
        f.get();
    }
    EXPECT_EQ(counter.load(), N);
}

TEST(ThreadPoolTest, SubmitAfterDestruction) {
    ThreadPool pool(4);
    pool.shutdown();
    EXPECT_THROW({
        auto future = pool.submit([](int x, int y) { return x + y; }, 21, 21);
        future.get();
    }, std::runtime_error);
}

TEST(ThreadPoolTest, TaskExecutionPropagation) {
    ThreadPool pool(2);

    auto future = pool.submit([](){
        throw std::runtime_error("Internal test failure");
    });
    EXPECT_THROW({
    try {
        future.get();
    }catch(std::exception& e) {
        EXPECT_STREQ(e.what(), "Internal test failure");
        throw;
    }
    }, std::runtime_error);
}

TEST(ThreadPoolTest, SingleThreadedPool) {
    ThreadPool pool(1);
    constexpr int N = 100;
    std::atomic<int> arr[100];
    std::vector<std::future<void>> futures;

    for (int i = 0; i < N; i++) {
        futures.push_back(pool.submit([&arr, i]() {
            arr[i] = i;
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    for (int j = 0; j < N; j++) {
        EXPECT_EQ(arr[j], j);
    }

}

int add(int a, int b) {return a+b;}
int sub(int a, int b) {return a-b;}
struct Multiplicaton {
    double operator()(int a, int b) {return a*b;}
};

TEST(ThreadPoolTest, ReturnTypeDeduction) {
    ThreadPool pool(4);

    auto future_add = pool.submit(add, 1, 2);
    auto future_sub = pool.submit(sub, 2, 1);
    auto future_mult = pool.submit(Multiplicaton{}, 2, 2);
    auto future_stringet = pool.submit([](const std::string& s)
        {return s + " world!";}, "Hello");

    EXPECT_EQ(future_add.get(), 3);
    EXPECT_EQ(future_sub.get(), 1);
    EXPECT_EQ(future_mult.get(), 4);
    EXPECT_EQ(future_stringet.get(), "Hello world!");
}

TEST(ThreadPoolTest, DestructorDrainsQueue) {
    ThreadPool pool(1);
    std::atomic<int> counter(0);

    pool.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    for (int i = 0; i < 5; i++) {
        pool.submit([&counter]() {
            counter++;
        });
    }
    pool.shutdown();
    EXPECT_EQ(counter, 5);
}

TEST(ThreadPoolTest, RaceTest) {
    ThreadPool pool(8);
    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};


    std::thread submitter([&]() {
        for (int i = 0; i < 1000; ++i) {
            try {
                pool.submit([&]() { successCount++; });
            } catch (const std::runtime_error&) {
                failCount++;
            }
        }
    });

    std::thread destroyer([&]() {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        pool.shutdown();
    });

    submitter.join();
    destroyer.join();

    EXPECT_EQ(successCount + failCount, 1000);
}
