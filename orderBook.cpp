#include <stdlib.h>
#include <iostream>
#include <map>
#include <deque>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <string>
#include <chrono>
#include <unistd.h>
#include <thread> 
#include <mutex>

enum OrderType {LIMIT, MARKET, STOP_LOSS, TAKE_PROFIT};


// efficiently constructing orders
struct Order
{
    size_t id_;
    std::chrono::time_point<std::chrono::system_clock> timestamp_;
    OrderType type_;
    size_t quantity_;
    bool isBuy_; 
    double price_;
    bool toRemove_;

    Order(size_t id, OrderType type, size_t quantity, bool isBuy, double price) : id_(id), timestamp_(std::chrono::system_clock::now()),
    type_(type), quantity_(quantity), isBuy_(isBuy), price_(price), toRemove_(false){}
};

// efficiently logging trades
struct Trade 
{
    int tradeId_;
    int buyOrderId_;
    int sellOrderId_;
    std::chrono::time_point<std::chrono::system_clock> timestamp_;
    size_t quantity_;
    double price_;

    Trade(size_t tradeId, int buyOrderId, int sellOrderId, size_t quantity, double price)
        : tradeId_(tradeId), buyOrderId_(buyOrderId), sellOrderId_(sellOrderId),
          timestamp_(std::chrono::system_clock::now()), quantity_(quantity), price_(price) {}
};


class OrderBook {
    private: 
    std::map<double, std::deque<Order>, std::greater<double>> bids_; 
    std::map<double, std::deque<Order>> ask_;
    std::vector<Trade> trades_;
    size_t trades = 0;
    size_t IDLevel_ = 0;


    public: 

    void addOrder(OrderType type , size_t quantity, bool isBuy, double price){
        Order order(IDLevel_++, type, quantity, isBuy, price);
        matchOrder(order);

        if(order.isBuy_ && order.type_ == LIMIT && order.quantity_ > 0) {
            bids_[order.price_].push_back(order);
        } else if(!order.isBuy_ && order.type_ == LIMIT && order.quantity_ > 0) {
           ask_[order.price_].push_back(order);
        }
    }

    void removeFromBooks(int removals, bool isBuy, double price) {
        if(isBuy) {
            while(removals != 0) {
                ask_[price].pop_front();
                removals--;
            }
        } else {
            while(removals != 0) {
                bids_[price].pop_front();
                removals--;
            }
        }
    }

    void removeMarket(int removals, bool isBuy, Order& order) {
        if(isBuy) {
            for(auto& ask : ask_) {
                while(!ask.second.empty() && ask.second.front().toRemove_ == true) ask.second.pop_front();
            }
        } else {
            for(auto& bid : bids_) {
                while(!bid.second.empty() && bid.second.front().toRemove_ == true) bid.second.pop_front();
            }
        }
        

        
    }
    void matchOrder(Order& order){
        // add to trades if a match occurs
        size_t originalQuantity = order.quantity_;
        int removals = 0;
        if(order.type_ == LIMIT) {
             if(order.isBuy_ && ask_.find(order.price_) != ask_.end()){
            // iterate through the deque and decrement our and their quantity, if we run out on that level, add to ask mp
                for(auto& ask : ask_[order.price_]) {
                    if(order.quantity_ <= 0) {
                        break;
                    }
                    if(ask.quantity_ == order.quantity_) {
                        // perfect match;
                        trades++;
                        trades_.emplace_back(trades, order.id_, ask.id_, order.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        order.quantity_ -= ask.quantity_;
                    } else if(ask.quantity_ > order.quantity_) {
                        // decrement ask
                        ask.quantity_ -= order.quantity_;
                        trades++;
                        trades_.emplace_back(trades, order.id_, ask.id_, order.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                    } else {
                        // remove ask
                        order.quantity_ -= ask.quantity_;
                        trades++;
                        trades_.emplace_back(trades, order.id_, ask.id_, ask.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        removals++;
                    }
                   
                }
            } else if (!order.isBuy_ && bids_.find(order.price_) != bids_.end()) {
                for(auto& bid : bids_[order.price_]) {
                    if(order.quantity_ <= 0) {
                        break;
                    }
                    if(bid.quantity_ == order.quantity_) {
                        trades++;
                        trades_.emplace_back(trades, order.id_, bid.id_,order.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        order.quantity_ -= bid.quantity_;
                    } else if(bid.quantity_ > order.quantity_) {
                        // decrement bid
                        bid.quantity_ -= order.quantity_;
                        trades++;
                        trades_.emplace_back(trades, order.id_, bid.id_, order.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        order.quantity_ -= order.quantity_;
                    } else {
                        // remove bid 
                        order.quantity_ -= bid.quantity_;
                        trades++;
                        trades_.emplace_back(trades, order.id_, bid.id_, bid.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        removals++;
                    }
                }
            }
            removeFromBooks(removals, order.isBuy_, order.price_);    
            return;
        }
       
        if(order.isBuy_) {
            // look in all of asks
            for(auto& it :ask_) {
                for(auto& ask : it.second) {
                    if(order.quantity_ <= 0) {
                        break;
                    }
                    if(ask.quantity_ == order.quantity_) {
                        // perfect match;
                        trades++;
                        trades_.emplace_back(trades, order.id_, ask.id_, order.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        order.quantity_ -= order.quantity_;
                        ask.toRemove_ = true;
                    } else if(ask.quantity_ > order.quantity_) {
                        // decrement ask
                        ask.quantity_ -= order.quantity_;
                        trades++;
                        trades_.emplace_back(trades, order.id_, ask.id_, order.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        order.quantity_ -= order.quantity_;
                    } else {
                        // remove ask
                        order.quantity_ -= ask.quantity_;
                        trades++;
                        trades_.emplace_back(trades, order.id_, ask.id_, ask.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        removals++;
                        ask.toRemove_ = true;
                    }
                    if(order.quantity_ <= 0) {
                        break;
                    }
                }
            }

        } else {
            for(auto& it : bids_) {
                for(auto& bid : bids_[order.price_]) {
                    if(order.quantity_ <= 0) {
                        break;
                    }
                    if(bid.quantity_ == order.quantity_) {
                        trades++;
                        trades_.emplace_back(trades, order.id_, bid.id_,order.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        order.quantity_ -= bid.quantity_;
                        bid.toRemove_ = true;
                    } else if(bid.quantity_ > order.quantity_) {
                        // decrement bid
                        bid.quantity_ -= order.quantity_;
                        trades++;
                        trades_.emplace_back(trades, order.id_, bid.id_, order.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        order.quantity_ -= order.quantity_;
                    } else {
                        // remove bid 
                        order.quantity_ -= bid.quantity_;
                        trades++;
                        trades_.emplace_back(trades, order.id_, bid.id_, bid.quantity_, order.price_);
                        printTrade(trades_[trades_.size() - 1]);
                        removals++;
                        bid.toRemove_ = true;
                    }
                    if(order.quantity_ == 0) {
                        break;
                    }
                }
            }
        }
        removeMarket(removals, order.isBuy_, order);
    }

    

    void printOrderBooks() {
        for(auto& bid : bids_) {
            for(auto& order : bid.second) {
                printOrder(order);
            }
        }

        for(auto& ask : ask_) {
            for(auto& order : ask.second) {
                printOrder(order);
            }
        }
    }

    void logTrade() {
        for (const auto& trade : trades_) {
            auto timestamp = std::chrono::system_clock::to_time_t(trade.timestamp_);
            std::cout << "Trade ID: " << trade.tradeId_
                    << " | Timestamp: " << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S")
                    << " | Price: " << trade.price_
                    << " | Quantity: " << trade.quantity_
                    << " | Buy Order ID: " << trade.buyOrderId_
                    << " | Sell Order ID: " << trade.sellOrderId_ << "\n";
        }
        trades_.clear();
    }

    void printTrade(Trade trade) {
        auto timestamp = std::chrono::system_clock::to_time_t(trade.timestamp_);
            std::cout << "Trade ID: " << trade.tradeId_
                    << " | Timestamp: " << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S")
                    << " | Price: " << trade.price_
                    << " | Quantity: " << trade.quantity_
                    << " | Buy Order ID: " << trade.buyOrderId_
                    << " | Sell Order ID: " << trade.sellOrderId_ << "\n";
    }


    void printOrder(const Order& order) {
        auto timestamp = std::chrono::system_clock::to_time_t(order.timestamp_);

        std::cout << "Order ID: " << order.id_ << "\n"
                << "Timestamp: " << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S") << "\n"
                << "Type: " << (order.type_ == LIMIT ? "LIMIT" : 
                                order.type_ == MARKET ? "MARKET" : 
                                order.type_ == STOP_LOSS ? "STOP_LOSS" : "TAKE_PROFIT") << "\n"
                << "Quantity: " << order.quantity_ << "\n"
                << "Side: " << (order.isBuy_ ? "Buy" : "Sell") << "\n"
                << "Price: " << order.price_ << "\n"
                << "----------------------------" << std::endl;
    }

};



void testerFunction(OrderBook& orderBook, OrderType type) {
    // Block 1: Basic limit orders at various prices and quantities
    sleep(1);
    orderBook.addOrder(type, 100, true, 100.0);    // Buy 100 shares at $100
    orderBook.addOrder(type, 150, true, 101.0);    // Buy 150 shares at $101
    orderBook.addOrder(type, 200, false, 100.0);   // Sell 200 shares at $100 (should match with order 1)
    orderBook.addOrder(type, 50, true, 102.0);     // Buy 50 shares at $102
    orderBook.addOrder(type, 80, false, 102.0);    // Sell 80 shares at $102 (partial match with order 4)

    sleep(1);
    // Block 2: Higher volume trades
    orderBook.addOrder(type, 500, true, 99.0);     // Buy 500 shares at $99
    orderBook.addOrder(type, 400, false, 99.0);    // Sell 400 shares at $99 (partial match with order 6)
    orderBook.addOrder(type, 300, true, 98.0);     // Buy 300 shares at $98
    orderBook.addOrder(type, 600, false, 100.0);   // Sell 600 shares at $100 (matches with remaining of order 1 and partially with order 2)
    
    sleep(1);
    // Block 3: Orders to check large buy and sell queues at same price
    orderBook.addOrder(type, 1000, true, 100.0);  // Buy 1000 shares at $100
    orderBook.addOrder(type, 1000, false, 100.0); // Sell 1000 shares at $100 (matches with previous order)
    orderBook.addOrder(type, 100, false, 100.0);  // Sell 100 shares at $100 (should remain as unmatched)

    sleep(1);
    // Block 4: High price variation with partial fills and different quantities
    orderBook.addOrder(type, 50, true, 105.0);    // Buy 50 shares at $105
    sleep(1);
    orderBook.addOrder(type, 150, false, 105.0);  // Sell 150 shares at $105 (partial match with previous order)
    orderBook.addOrder(type, 200, true, 104.0);   // Buy 200 shares at $104
    sleep(1);
    orderBook.addOrder(type, 300, false, 104.0);  // Sell 300 shares at $104 (partial match with previous order)

    sleep(1);
    // Block 5: Multiple price levels
    for (int i = 0; i < 5; ++i) {
        orderBook.addOrder(type, 100 + i, true, 103.0 + i);    // Buy orders at prices 103 to 107
        sleep(1);
        orderBook.addOrder(type, 150 + i, false, 103.0 + i); // Sell orders at matching prices 103 to 107
    }

    sleep(1);
    // Block 6: High-volume buys and sells
    orderBook.addOrder(type, 2000, true, 98.0);   // Buy 2000 shares at $98
    orderBook.addOrder(type, 2500, false, 98.0);  // Sell 2500 shares at $98 (partially matches previous order)

    sleep(1);
    // Block 7: Simulating large price jumps and partial matches
    orderBook.addOrder(type, 700, true, 90.0);    // Buy 700 shares at $90
    orderBook.addOrder(type, 800, false, 91.0);   // Sell 800 shares at $91
    orderBook.addOrder(type, 300, false, 90.0);   // Sell 300 shares at $90 (matches with previous order)
    orderBook.addOrder(type, 500, true, 91.0);    // Buy 500 shares at $91 (matches with previous order)

    sleep(1);
    // Block 8: Orders at low quantities
    for (int i = 0; i < 5; ++i) {
        orderBook.addOrder(type, 5, true, 95.0);    // Small buy orders at $95
        sleep(1);
        orderBook.addOrder(type, 5, false, 95.0);   // Small sell orders at $95
    }

    sleep(1);
    // Block 9: High-volume trades to clear orders at $100
    orderBook.addOrder(type, 5000, true, 100.0);  // High-volume buy at $100
    orderBook.addOrder(type, 5000, false, 100.0); // High-volume sell at $100 (matches with previous order)

    sleep(1);
    // Block 10: Increasing complexity with buy/sell mismatches at various price levels
    for (int i = 0; i < 10; ++i) {
        orderBook.addOrder(type, 250, i % 2 == 0, 99.0 + (i % 5)); // Alternating buy/sell at prices from 99 to 103
        sleep(1);
    }
}


int main() {
    OrderBook orderBook;

    for(int i = 0; i < 10; i++) {
        OrderType type = i < 6 ? LIMIT : MARKET;
        testerFunction(orderBook, type);
    }
   

    // Print the order book after all additions
    std::cout << "\n--- Order Book (Large Combined Test Case) ---\n";
    orderBook.printOrderBooks();

    // Print the trade log to see all matched trades
    std::cout << "\n--- Trade Log (Large Combined Test Case) ---\n";
    orderBook.logTrade();

    // Check the trade log again to ensure it is cleared after printing
    std::cout << "\n--- Trade Log (After Clearing) ---\n";
    orderBook.logTrade();

    return 0;
}
