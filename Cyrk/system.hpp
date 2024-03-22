#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <thread>
#include <iostream>
#include <exception>
#include <chrono>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <mutex>
#include <functional>
#include <future>
#include "machine.hpp"

//***************************************************
//**               STRUCTS                         **
//***************************************************

struct pojemnik{
    std::condition_variable cv;
    std::mutex queue_mutex;
    std::mutex order_mutex;
};

struct pagers_data{
    std::condition_variable *cv_taken;
    std::condition_variable *cv_wait;

    std::mutex *mutex_wait;
    std::mutex *mutex_taken;

    std::vector<std::unique_ptr<Product>> *products;
    bool *is_ready;
    bool *failed;
    bool *taken;
    bool *expired;

    unsigned int id;
};


//***************************************************
//**               STRUCTS                         **
//***************************************************

class Menu {
public:
    Menu() = default;

    bool contains(std::string record){
        m.lock();
        bool check = false;
        for(auto & r: menu){
            if(r == record){check = true;}
        }
        m.unlock();
        return check;
    }

    void remove_record(std::string record){
        m.lock();
        erase(menu, record);
        m.unlock();
    }

    void add_record(std::string record){
        m.lock();
        menu.push_back(record);
        m.unlock();
    }

    void make_empty(){
        menu.clear();
    }

    std::vector<std::string> menu;
    std::mutex m;
};

class PendingOrders {
public:
    PendingOrders() = default;

    void remove_id(unsigned int id){
        m.lock();
        erase(pending_orders, id);
        m.unlock();
    }

    void add_id(unsigned int id){
        m.lock();
        pending_orders.push_back(id);
        m.unlock();
    }

    std::vector<unsigned int> pending_orders;
    std::mutex m;
};


//***************************************************
//**               FAIR MUTEX                      **
//***************************************************


class FairMutex {
    std::mutex mutex;
    std::condition_variable cv_;
    unsigned int next_, curr_;

public:
    FairMutex() : next_(0), curr_(0) {}
    ~FairMutex() = default;

    FairMutex(const FairMutex&) = delete;
    FairMutex& operator=(const FairMutex&) = delete;

    void lock()
    {
        std::unique_lock<std::mutex> lk(mutex);
        const unsigned int self = next_++;
        cv_.wait(lk, [&]{ return (self == curr_); });
    }
    bool try_lock()
    {
        std::lock_guard<std::mutex> lk(mutex);
        if (next_ != curr_)
            return false;
        ++next_;
        return true;
    }
    void unlock()
    {
        std::lock_guard<std::mutex> lk(mutex);
        ++curr_;
        cv_.notify_all();
    }
};

//***************************************************
//**               EXCEPTIONS                      **
//***************************************************

class FulfillmentFailure : public std::exception
{
};

class OrderNotReadyException : public std::exception
{
};

class BadOrderException : public std::exception
{
};

class BadPagerException : public std::exception
{
};

class OrderExpiredException : public std::exception
{
};

class RestaurantClosedException : public std::exception
{
};

//***************************************************
//**                  WORKER REPORT                **
//***************************************************

struct WorkerReport
{
    std::vector<std::vector<std::string>> collectedOrders;
    std::vector<std::vector<std::string>> abandonedOrders;
    std::vector<std::vector<std::string>> failedOrders;
    std::vector<std::string> failedProducts;
};

//***************************************************
//**               COASTER PAGER                   **
//***************************************************

class CoasterPager
{
public:
    typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t;
    typedef std::unordered_map<std::string, std::unique_ptr<FairMutex>> mutex_t;
    typedef std::queue<std::pair<std::vector<std::string>, pagers_data>> queue_t;

    void wait() const;

    void wait(unsigned int timeout) const;

    [[nodiscard]] unsigned int getId() const;

    [[nodiscard]] bool isReady() const;


private:
    explicit CoasterPager(unsigned int new_id);

    CoasterPager() = default;

    unsigned int id;

    bool failed;
    bool is_ready;
    mutable bool taken;
    bool expired;

    std::condition_variable cv_taken;
    std::mutex mutex_taken;

    mutable std::condition_variable cv_wait;
    mutable std::mutex mutex_wait;

    std::vector<std::unique_ptr<Product>> products;

    friend class System;
};


//***************************************************
//**                  SYSTEM                       **
//***************************************************

class System
{
public:
    typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t;

    System(machines_t machines_in, unsigned int numberOfWorkers_in, unsigned int clientTimeout_in);

    std::vector<WorkerReport> shutdown();

    std::vector<std::string> getMenu() const;

    std::vector<unsigned int> getPendingOrders() const;

    std::unique_ptr<CoasterPager> order(std::vector<std::string> products);

    std::vector<std::unique_ptr<Product>> collectOrder(std::unique_ptr<CoasterPager> CoasterPager);

    unsigned int getClientTimeout() const;

private:
    typedef std::unordered_map<std::string, std::unique_ptr<FairMutex>> mutex_t;
    typedef std::queue<std::pair<std::vector<std::string>, pagers_data>> queue_t;

    std::vector<std::jthread> workers;

    std::shared_ptr<machines_t> machines;

    pojemnik dane;

    mutex_t machines_mutexes;

    bool closed;
    unsigned int clientTimeout;
    unsigned int id = 0;

    std::vector<WorkerReport> workers_reports;
    PendingOrders pending_orders;

    Menu menu;

    queue_t queue_orders;
};

#endif // SYSTEM_HPP