#include "order/order_book.hpp"
#include "core/time.hpp"

namespace order {

using namespace core;

OrderBook::OrderBook() : pool_(std::make_unique<core::memory::ObjectPool<Order, 65536>>()) {
    best_bid_ = bids_.end();
    best_ask_ = asks_.end();
}

void OrderBook::refresh_best_bid_ask(Side side, Price price) {
    if (side == Side::Buy) {
        if (best_bid_ == bids_.end() || price > best_bid_->first) {
            best_bid_ = bids_.begin();
        }
    } else {
        if (best_ask_ == asks_.end() || price < best_ask_->first) {
            best_ask_ = asks_.begin();
        }
    }
}

void OrderBook::refresh_best_bid_ask() {
    best_bid_ = bids_.begin();
    best_ask_ = asks_.begin();
}

Order* OrderBook::add(OrderId id, Side side, Price price, Quantity qty) {
    Order* o = pool_->allocate();
    if (!o) return nullptr;

    o->id = id;
    o->price = price;
    o->qty = qty;
    o->remaining_qty = qty;
    o->side = side;
    o->type = OrderType::Limit;
    o->state = OrderState::New;
    o->timestamp_ns = core::time::MonotonicClock::now_ns();
    o->prev = nullptr;
    o->next = nullptr;

    order_map_[id] = o;

    if (side == Side::Buy) {
        bids_[price].price = price;
        bids_[price].push_back(o);
    } else {
        asks_[price].price = price;
        asks_[price].push_back(o);
    }

    refresh_best_bid_ask(side, price);
    return o;
}

void OrderBook::add_resting(Order* order) {
    order_map_[order->id] = order;

    if (order->side == Side::Buy) {
        bids_[order->price].price = order->price;
        bids_[order->price].push_back(order);
    } else {
        asks_[order->price].price = order->price;
        asks_[order->price].push_back(order);
    }

    refresh_best_bid_ask(order->side, order->price);
}

bool OrderBook::cancel(OrderId id) {
    auto it = order_map_.find(id);
    if (it == order_map_.end()) return false;

    Order* o = it->second;
    
    // Prevent double-free
    if (o->state == OrderState::Cancelled || o->state == OrderState::Filled) {
        return false;
    }

    o->state = OrderState::Cancelled;

    if (o->side == Side::Buy) {
        auto level_it = bids_.find(o->price);
        if (level_it != bids_.end()) {
            level_it->second.remove(o);
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        auto level_it = asks_.find(o->price);
        if (level_it != asks_.end()) {
            level_it->second.remove(o);
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
        }
    }

    order_map_.erase(it);
    pool_->deallocate(o);
    refresh_best_bid_ask();

    return true;
}

bool OrderBook::modify(OrderId id, Quantity new_qty) {
    // Modify is cancel-then-add. This loses price-time priority.
    // TODO: Special case: if only qty is reduced (not price change),
    // some exchanges allow in-place qty reduction without losing priority.
    
    auto it = order_map_.find(id);
    if (it == order_map_.end()) return false;
    
    Order* o = it->second;
    Side side = o->side;
    Price price = o->price;
    
    if (cancel(id)) {
        return add(id, side, price, new_qty) != nullptr;
    }
    return false;
}

} // namespace order
