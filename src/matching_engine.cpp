#include "exchange/matching_engine.hpp"

#include <algorithm>

namespace exchange {
MatchingEngine::MatchingEngine(OrderManager& order_manager, ExecutionReporter& reporter)
    : order_manager_(order_manager), reporter_(reporter) {}

OrderBook& MatchingEngine::getOrCreateBook(std::string_view symbol) {
    std::string key(symbol);
    auto it = books_.find(key);
    if (it != books_.end()) {
        return *(it->second);
    }
    auto book = std::make_unique<OrderBook>(symbol);
    OrderBook* raw = book.get();
    books_.emplace(std::move(key), std::move(book));
    return *raw;
}

OrderBook* MatchingEngine::getBook(std::string_view symbol) {
    auto it = books_.find(std::string(symbol));
    if (it != books_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const OrderBook* MatchingEngine::getBook(std::string_view symbol) const {
    auto it = books_.find(std::string(symbol));
    if (it != books_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void MatchingEngine::processOrder(Order* order) {
    if (!order) {
        return;
    }

    if (order->symbol.empty() || order->quantity == 0) {
        order->status = OrderStatus::Rejected;
        reporter_.reportOrderRejected(order->id, order->symbol, "Invalid order parameters");
        return;
    }

    if (order->type == OrderType::Limit && order->price <= 0) {
        order->status = OrderStatus::Rejected;
        reporter_.reportOrderRejected(order->id, order->symbol, "Price must be positive for Limit orders");
        return;
    }

    reporter_.reportOrderAccepted(*order);
    auto& book = getOrCreateBook(order->symbol);

    if (order->side == Side::Buy) {
        matchBuyOrder(order, book);
    } else {
        matchSellOrder(order, book);
    }
}

void MatchingEngine::matchBuyOrder(Order* incoming, OrderBook& book) {
    while (incoming->remaining() > 0) {
        Order* best_ask = book.peekBestAsk();
        if (!best_ask) {
            break;
        }

        if (incoming->type == OrderType::Limit && incoming->price < best_ask->price) {
            break;
        }

        Quantity trade_qty = std::min(incoming->remaining(), best_ask->remaining());

        Price trade_price = best_ask->price;
        Timestamp trade_time = (incoming->timestamp != 0) ? incoming->timestamp : currentNanos();

        incoming->filled_quantity += trade_qty;
        best_ask->filled_quantity += trade_qty;

        Trade trade{
            .buy_order_id = incoming->id,
            .sell_order_id = best_ask->id,
            .symbol = incoming->symbol,
            .price = trade_price,
            .quantity = trade_qty,
            .timestamp = trade_time
        };
        reporter_.reportTrade(trade);

        if (best_ask->isFilled()) {
            best_ask->status = OrderStatus::Filled;
            book.removeOrder(best_ask->id);
            reporter_.reportOrderFilled(*best_ask);
        } else {
            best_ask->status = OrderStatus::PartiallyFilled;
            reporter_.reportOrderPartiallyFilled(*best_ask, trade_qty);
        }
    }

    if (incoming->isFilled()) {
        incoming->status = OrderStatus::Filled;
        reporter_.reportOrderFilled(*incoming);
    } else {
        if (incoming->type == OrderType::Limit) {
            incoming->status = (incoming->filled_quantity > 0)
                                   ? OrderStatus::PartiallyFilled
                                   : OrderStatus::New;
            if (incoming->filled_quantity > 0) {
                reporter_.reportOrderPartiallyFilled(*incoming, 0);
            }
            book.addOrder(incoming);
        } else {
            incoming->status = (incoming->filled_quantity > 0)
                                   ? OrderStatus::PartiallyFilled
                                   : OrderStatus::Cancelled;
            reporter_.reportOrderCancelled(incoming->id, incoming->symbol, incoming->remaining());
        }
    }
}

void MatchingEngine::matchSellOrder(Order* incoming, OrderBook& book) {
    while (incoming->remaining() > 0) {
        Order* best_bid = book.peekBestBid();
        if (!best_bid) {
            break;
        }

        if (incoming->type == OrderType::Limit && incoming->price > best_bid->price) {
            break;
        }

        Quantity trade_qty = std::min(incoming->remaining(), best_bid->remaining());

        Price trade_price = best_bid->price;
        Timestamp trade_time = (incoming->timestamp != 0) ? incoming->timestamp : currentNanos();

        incoming->filled_quantity += trade_qty;
        best_bid->filled_quantity += trade_qty;

        Trade trade{
            .buy_order_id = best_bid->id,
            .sell_order_id = incoming->id,
            .symbol = incoming->symbol,
            .price = trade_price,
            .quantity = trade_qty,
            .timestamp = trade_time
        };
        reporter_.reportTrade(trade);

        if (best_bid->isFilled()) {
            best_bid->status = OrderStatus::Filled;
            book.removeOrder(best_bid->id);
            reporter_.reportOrderFilled(*best_bid);
        } else {
            best_bid->status = OrderStatus::PartiallyFilled;
            reporter_.reportOrderPartiallyFilled(*best_bid, trade_qty);
        }
    }

    if (incoming->isFilled()) {
        incoming->status = OrderStatus::Filled;
        reporter_.reportOrderFilled(*incoming);
    } else {
        if (incoming->type == OrderType::Limit) {
            incoming->status = (incoming->filled_quantity > 0)
                                   ? OrderStatus::PartiallyFilled
                                   : OrderStatus::New;
            if (incoming->filled_quantity > 0) {
                reporter_.reportOrderPartiallyFilled(*incoming, 0);
            }
            book.addOrder(incoming);
        } else {
            incoming->status = (incoming->filled_quantity > 0)
                                   ? OrderStatus::PartiallyFilled
                                   : OrderStatus::Cancelled;
            reporter_.reportOrderCancelled(incoming->id, incoming->symbol, incoming->remaining());
        }
    }
}

bool MatchingEngine::cancelOrder(OrderId id) {
    Order* order = order_manager_.getOrder(id);
    if (!order) {
        return false;
    }

    if (order->status != OrderStatus::New && order->status != OrderStatus::PartiallyFilled) {
        return false;
    }

    auto* book = getBook(order->symbol);
    if (!book) {
        return false;
    }

    Quantity remaining = order->remaining();
    if (!book->removeOrder(id)) {
        return false;
    }

    order->status = OrderStatus::Cancelled;
    reporter_.reportOrderCancelled(id, order->symbol, remaining);
    return true;
}

void MatchingEngine::clear() {
    books_.clear();
}
}
