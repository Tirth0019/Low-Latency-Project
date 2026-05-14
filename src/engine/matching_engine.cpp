#include "engine/matching_engine.hpp"
#include "core/time.hpp"
#include <algorithm>

namespace engine {

std::vector<TradeEvent> MatchingEngine::process_new_order(OrderId id, Side side, Price price, Quantity qty) {
    // 1. Allocate a temporary aggressive order (we allocate from the pool so it can be rested later)
    Order* aggressive = book_.pool_->allocate();
    if (!aggressive) {
        return {}; // Pool exhausted
    }

    aggressive->id = id;
    aggressive->price = price;
    aggressive->qty = qty;
    aggressive->remaining_qty = qty;
    aggressive->side = side;
    aggressive->type = OrderType::Limit;
    aggressive->state = OrderState::New;
    aggressive->timestamp_ns = core::time::MonotonicClock::now_ns();
    aggressive->prev = nullptr;
    aggressive->next = nullptr;

    return match(aggressive);
}

bool MatchingEngine::process_cancel_order(OrderId id) {
    return book_.cancel(id);
}

std::vector<TradeEvent> MatchingEngine::match(Order* aggressive) {
    std::vector<TradeEvent> trades;
    
    // Asymmetric book access
    // If aggressive is Buy, it hits Asks.
    // If aggressive is Sell, it hits Bids.
    bool hits_asks = (aggressive->side == Side::Buy);

    while (aggressive->remaining_qty > Quantity(0)) {
        if (hits_asks) {
            if (book_.asks_.empty()) break;
        } else {
            if (book_.bids_.empty()) break;
        }

        // We can't auto& [best_price, best_level] cleanly with branching without duplication, 
        // but we can use an iterator. Let's do it explicitly as the plan implies.
        Price best_price;
        PriceLevel* best_level_ptr = nullptr;

        if (hits_asks) {
            auto it = book_.asks_.begin();
            best_price = it->first;
            best_level_ptr = &it->second;
            
            if (aggressive->price < best_price) break; // Cross condition fails
        } else {
            auto it = book_.bids_.begin();
            best_price = it->first;
            best_level_ptr = &it->second;
            
            if (aggressive->price > best_price) break; // Cross condition fails
        }

        PriceLevel& best_level = *best_level_ptr;
        Order* passive = best_level.head; // FIFO: oldest order first

        Quantity fill_qty = Quantity(std::min((uint64_t)aggressive->remaining_qty, (uint64_t)passive->remaining_qty));

        // emit trade
        trades.push_back({
            aggressive->id, 
            passive->id,
            best_price, 
            fill_qty, 
            core::time::MonotonicClock::now_ns()
        });

        // update quantities
        aggressive->remaining_qty = Quantity((uint64_t)aggressive->remaining_qty - (uint64_t)fill_qty);
        passive->remaining_qty    = Quantity((uint64_t)passive->remaining_qty - (uint64_t)fill_qty);
        best_level.total_qty      -= (uint64_t)fill_qty;

        // update states
        passive->state = ((uint64_t)passive->remaining_qty == 0)
                         ? OrderState::Filled : OrderState::PartFill;

        // if passive fully filled: remove from book + deallocate
        if ((uint64_t)passive->remaining_qty == 0) {
            best_level.remove(passive);
            book_.order_map_.erase(passive->id);
            book_.pool_->deallocate(passive);
            
            if (best_level.empty()) {
                if (hits_asks) {
                    book_.asks_.erase(book_.asks_.begin());
                } else {
                    book_.bids_.erase(book_.bids_.begin());
                }
            }
        }
    }

    // handle aggressive remainder
    aggressive->state = ((uint64_t)aggressive->remaining_qty == 0)
                        ? OrderState::Filled : OrderState::PartFill;
                        
    if ((uint64_t)aggressive->remaining_qty > 0) {
        book_.add_resting(aggressive); // rest unfilled portion
    } else {
        book_.pool_->deallocate(aggressive); // fully filled aggressive doesn't rest
    }

    book_.refresh_best_bid_ask();
    return trades;
}

} // namespace engine
