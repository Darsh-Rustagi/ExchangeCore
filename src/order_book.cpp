#include "exchange/order_book.hpp"

namespace exchange {
OrderBook::OrderBook(std::string_view symbol)
    : symbol_(symbol) {}

void OrderBook::addOrder(Order* order) {
    if (!order || order->remaining() == 0) {
        return;
    }

    if (order_locations_.find(order->id) != order_locations_.end()) {
        removeOrder(order->id);
    }

    if (order->side == Side::Buy) {
        auto& list = bids_[order->price];
        list.push_back(order);
        auto it = std::prev(list.end());
        order_locations_[order->id] = OrderLocation{Side::Buy, order->price, it};
    } else {
        auto& list = asks_[order->price];
        list.push_back(order);
        auto it = std::prev(list.end());
        order_locations_[order->id] = OrderLocation{Side::Sell, order->price, it};
    }
}

bool OrderBook::removeOrder(OrderId id) {
    auto loc_it = order_locations_.find(id);
    if (loc_it == order_locations_.end()) {
        return false;
    }

    const auto& loc = loc_it->second;
    if (loc.side == Side::Buy) {
        auto map_it = bids_.find(loc.price);
        if (map_it != bids_.end()) {
            map_it->second.erase(loc.iterator);
            if (map_it->second.empty()) {
                bids_.erase(map_it);
            }
        }
    } else {
        auto map_it = asks_.find(loc.price);
        if (map_it != asks_.end()) {
            map_it->second.erase(loc.iterator);
            if (map_it->second.empty()) {
                asks_.erase(map_it);
            }
        }
    }

    order_locations_.erase(loc_it);
    return true;
}

std::optional<Price> OrderBook::bestBid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::bestAsk() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

Order* OrderBook::peekBestBid() {
    if (bids_.empty() || bids_.begin()->second.empty()) {
        return nullptr;
    }
    return bids_.begin()->second.front();
}

Order* OrderBook::peekBestAsk() {
    if (asks_.empty() || asks_.begin()->second.empty()) {
        return nullptr;
    }
    return asks_.begin()->second.front();
}

std::vector<PriceLevel> OrderBook::getBidDepth(std::size_t max_levels) const {
    std::vector<PriceLevel> depth;
    depth.reserve(std::min(max_levels, bids_.size()));

    std::size_t count = 0;
    for (const auto& [price, orders] : bids_) {
        if (count++ >= max_levels) break;
        Quantity total_qty = 0;
        for (const auto* ord : orders) {
            total_qty += ord->remaining();
        }
        depth.push_back(PriceLevel{price, total_qty, orders.size()});
    }
    return depth;
}

std::vector<PriceLevel> OrderBook::getAskDepth(std::size_t max_levels) const {
    std::vector<PriceLevel> depth;
    depth.reserve(std::min(max_levels, asks_.size()));

    std::size_t count = 0;
    for (const auto& [price, orders] : asks_) {
        if (count++ >= max_levels) break;
        Quantity total_qty = 0;
        for (const auto* ord : orders) {
            total_qty += ord->remaining();
        }
        depth.push_back(PriceLevel{price, total_qty, orders.size()});
    }
    return depth;
}

std::optional<Price> OrderBook::spread() const {
    auto bb = bestBid();
    auto ba = bestAsk();
    if (bb.has_value() && ba.has_value()) {
        return *ba - *bb;
    }
    return std::nullopt;
}

void OrderBook::clear() {
    bids_.clear();
    asks_.clear();
    order_locations_.clear();
}
}
