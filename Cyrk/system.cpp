#include "system.hpp"


typedef std::unordered_map<std::string, std::shared_ptr<Machine>> machines_t;
typedef std::unordered_map<std::string, std::unique_ptr<FairMutex>> mutex_t;
typedef std::queue<std::pair<std::vector<std::string>, pagers_data>> queue_t;


//***************************************************
//**               COASTER PAGER                   **
//***************************************************
CoasterPager::CoasterPager(unsigned int new_id) {
    id = new_id;
    is_ready = false;
    failed = false;
    expired = false;
    taken = false;
}

void CoasterPager::wait() const {
    std::unique_lock <std::mutex> lock(mutex_wait);
    cv_wait.wait(lock, [this] { return is_ready; });
    if(failed){
        taken = true;
        throw FulfillmentFailure();
    }
}

void CoasterPager::wait(const unsigned int timeout) const {
    std::unique_lock <std::mutex> lock(mutex_wait);
    auto now = std::chrono::system_clock::now();
    auto time_out = std::chrono::milliseconds(timeout);
    cv_wait.wait_until(lock, now + time_out, [this]() { return is_ready; });
    if(failed){
        taken = true;
        throw FulfillmentFailure();
    }
}

unsigned int CoasterPager::getId() const {
    return id;
}

bool CoasterPager::isReady() const {
    return is_ready;
}

//***************************************************
//**                  SYSTEM                       **
//***************************************************


void routine(const std::stop_token& stoken ,queue_t &queue_orders, std::shared_ptr<machines_t> machines, mutex_t
&machines_mutexes, pojemnik &dane, unsigned int clientTimeout, Menu &menu, PendingOrders &pending_orders, std::vector<WorkerReport> &workers_reports) {

    WorkerReport workerReport;

    while(!stoken.stop_requested()) {
//        std::cerr << "xd" << '\n';
        std::unique_lock<std::mutex> lock(dane.order_mutex);
        dane.cv.wait(lock, [&queue_orders, &stoken]{return stoken.stop_requested() || !queue_orders.empty();});

        if(stoken.stop_requested()){
//            std::cerr << "xddd" << '\n';
            break;
        }

        std::unique_lock<std::mutex> lock2(dane.queue_mutex);
        auto current_order = queue_orders.front().first;
        auto pager = queue_orders.front().second;

        std::vector<std::pair<std::thread, std::string>> threads_lock;

        for(auto & food: current_order){
            threads_lock.emplace_back(std::thread {[&]() {
                machines_mutexes[food]->lock();
            }}, food);
        }
        lock.unlock();

        queue_orders.pop();
        lock2.unlock();

        std::vector<std::thread> threads_order;
        std::vector<std::unique_ptr<Product>> products;
        std::vector<std::string> foods;
        bool if_execption = false;
        std::mutex m;

        for(auto & thread: threads_lock){
            threads_order.emplace_back(std::thread {[&]() {
                thread.first.join();
                std::string food = thread.second;

                std::unique_ptr<Product> produkt = nullptr;

                try{
                    auto product = (*machines)[food]->getProduct();

                    std::unique_lock<std::mutex> lock(m);
                    produkt = std::move(product);
                    products.push_back(std::move(produkt));
                    foods.push_back(food);
                    machines_mutexes[food]->unlock();
                    lock.unlock();
                } catch(std::exception& error) {
                    menu.remove_record(food);
                    if_execption = true;
                    workerReport.failedProducts.push_back(food);
                    machines_mutexes[food]->unlock();
                }
            }});
        }

        for(auto &thread : threads_order){
            thread.join();
        }

        if(if_execption){
            workerReport.failedOrders.push_back(current_order);
            std::atomic_uint it = 0;
            for(const auto& food : foods){
                if(products[it] != nullptr){
                    machines_mutexes[food]->lock();
                    (*machines)[food]->returnProduct(std::move(products[it]));
                    it++;
                    machines_mutexes[food]->unlock();
                }
            }

            std::unique_lock<std::mutex> lock3(*pager.mutex_wait);
            *pager.failed = true;
            *pager.is_ready = true;
            lock3.unlock();
            (*pager.cv_wait).notify_one();

            pending_orders.remove_id(pager.id);
        } else {
            std::unique_lock<std::mutex> lock3(*pager.mutex_wait);

            *pager.products = std::move(products);
            *pager.is_ready = true;
            auto now = std::chrono::system_clock::now();
            lock3.unlock();
            (*pager.cv_wait).notify_one();

            std::unique_lock<std::mutex> lock4(*pager.mutex_taken);
            auto timeout = std::chrono::milliseconds(clientTimeout);
            if((*pager.cv_taken).wait_until(lock4, now + timeout, [&]{return *pager.taken;})){
                workerReport.collectedOrders.push_back(current_order);
                pending_orders.remove_id(pager.id);
            } else {
                workerReport.abandonedOrders.push_back(current_order);
                *pager.expired = true;
                pending_orders.remove_id(pager.id);
                std::atomic_uint it = 0;
                for(const auto& food : foods){
                    if((*pager.products)[it] != nullptr){
                        machines_mutexes[food]->lock();
                        (*machines)[food]->returnProduct(std::move((*pager.products)[it]));
                        it++;
                        machines_mutexes[food]->unlock();
                    }
                }
            }
        }
    }
    std::unique_lock<std::mutex> lock2(dane.order_mutex);
    workers_reports.emplace_back(std::move(workerReport));
    dane.cv.notify_one();
}


System::System(machines_t machines_in, unsigned int numberOfWorkers, unsigned int clientTimeout_in) :
        machines(std::make_shared<machines_t>(std::move(machines_in))),
        clientTimeout(clientTimeout_in),
        pending_orders(),
        menu()
{
    closed = false;
    id = 0;

    for(const auto& part : *machines){
        part.second->start();
        machines_mutexes[part.first] = std::make_unique<FairMutex>();
        menu.add_record(part.first);
    }

    for(unsigned int i = 0;i < numberOfWorkers;i++){
//        workers_reports.emplace_back();
        workers.emplace_back(std::jthread {[&](const std::stop_token& stoken){
            routine(stoken, queue_orders, machines, machines_mutexes, std::ref(dane), clientTimeout, menu, pending_orders, workers_reports);
        }});
    }
}

std::vector<WorkerReport> System::shutdown() {

    menu.make_empty();
    for(const auto& machine : *machines){
        machine.second->stop();
    }
    std::unique_lock<std::mutex> lock(dane.order_mutex);
    closed = true;

    for(auto & worker : workers){
        worker.request_stop();
    }
    dane.cv.notify_all();
    lock.unlock();

    std::mutex m;
    std::unique_lock<std::mutex> lock2(m);
    dane.cv.wait(lock2, [&]{return workers.size() == workers_reports.size();});
    lock2.unlock();

//    for(auto &xd : workers_reports){
//        std::cout << "COLECTED ORDERS \n";
//        for(auto a : xd.collectedOrders){
//            for(auto i : a){
//                std::cout << i << " ";
//            }
//            std::cout << "\n";
//        }
//        std::cout << "FAILED ORDERS \n";
//        for(auto a : xd.failedOrders){
//            for(auto i : a){
//                std::cout << i << " ";
//            }
//            std::cout << "\n";
//        }
//        std::cout << "ABANDONED ORDERS \n";
//        for(auto a : xd.abandonedOrders){
//            for(auto i : a){
//                std::cout << i << " ";
//            }
//            std::cout << "\n";
//        }
//        std::cout << "FAILED PRDOUCTS \n";
//        for(auto a : xd.failedProducts){
//            std::cout << a << " ";
//            std::cout << "\n";
//        }
//
//    }

    return workers_reports;
}

std::unique_ptr<CoasterPager> System::order(std::vector<std::string> products){
    if(!closed){
        for(auto & food : products){
            if(!menu.contains(food)){
                throw BadOrderException();
            }
        }
        if(products.empty()){throw BadOrderException();}
        std::unique_lock<std::mutex> lock(dane.order_mutex);
        pagers_data p_data{};
        id++;
        auto * coaster_pager =  new CoasterPager(id);
        pending_orders.add_id(id);
        auto result = std::unique_ptr<CoasterPager>(coaster_pager);

        //branie referencji z coaster pagera
        p_data.mutex_wait = &result->mutex_wait;
        p_data.mutex_taken = &result->mutex_taken;

        p_data.cv_wait = &result->cv_wait;
        p_data.cv_taken = &result->cv_taken;

        p_data.products = &result->products;
        p_data.expired = &result->expired;
        p_data.is_ready = &result->is_ready;
        p_data.failed = &result->failed;
        p_data.taken = &result->taken;

        p_data.id = result->id;
        // koniec brania referencji

        queue_orders.push(std::make_pair(std::move(products), p_data));
        lock.unlock();
        dane.cv.notify_one();
//
        return result;
    } else {
        throw RestaurantClosedException();
    }

}

std::vector<std::unique_ptr<Product>> System::collectOrder(std::unique_ptr<CoasterPager> CoasterPager) {
    if(CoasterPager->failed){throw FulfillmentFailure();}
    if(!CoasterPager->isReady()){throw OrderNotReadyException();}
    if(CoasterPager->taken){throw BadOrderException();}
    if(CoasterPager->expired){throw OrderExpiredException();}

//    CoasterPager->wait();
    std::unique_lock<std::mutex> lock(CoasterPager->mutex_taken);
    CoasterPager->taken = true;
    lock.unlock();
    (CoasterPager->cv_taken).notify_one();
//    std::cerr << "XAXAX\n";

    return std::move(CoasterPager->products);
}

std::vector<unsigned int> System::getPendingOrders() const {
    return pending_orders.pending_orders;
}

std::vector<std::string> System::getMenu() const {
    return menu.menu;
}

unsigned int System::getClientTimeout() const {
    return clientTimeout;
}