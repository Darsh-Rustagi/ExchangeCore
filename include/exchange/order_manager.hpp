#pragma once

#include "exchange/types.hpp"
#include "exchange/order.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exchange {
class OrderManager {
public:
    OrderManager() = default;
    ~OrderManager() = default;

    OrderManager(const OrderManager&) = delete;
    OrderManager& operator=(const OrderManager&) = delete;
    OrderManager(OrderManager&&) noexcept = default;
    OrderManager& operator=(OrderManager&&) noexcept = default;

    Order* createOrder(Side side,
                       OrderType type,
                       std::string_view symbol,
                       Price price,
                       Quantity quantity,
                       Timestamp timestamp = 0,
                       std::optional<OrderId> custom_id = std::nullopt);

    [[nodiscard]] Order* getOrder(OrderId id);
    [[nodiscard]] const Order* getOrder(OrderId id) const;

    [[nodiscard]] bool hasOrder(OrderId id) const noexcept;

    [[nodiscard]] std::size_t totalOrders() const noexcept;

    void clear();

private:
    OrderId next_id_{1};

    std::unordered_map<OrderId, std::unique_ptr<Order>> orders_;
};
}
