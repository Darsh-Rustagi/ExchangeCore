#pragma once

#include "exchange/types.hpp"
#include <string>

namespace exchange {
struct Order {
    OrderId     id{0};
    std::string symbol{};
    Side        side{Side::Buy};
    OrderType   type{OrderType::Limit};
    Price       price{0};
    Quantity    quantity{0};
    Quantity    filled_quantity{0};
    OrderStatus status{OrderStatus::New};
    Timestamp   timestamp{0};

    [[nodiscard]] constexpr Quantity remaining() const noexcept {
        return (quantity > filled_quantity) ? (quantity - filled_quantity) : 0;
    }

    [[nodiscard]] constexpr bool isFilled() const noexcept {
        return filled_quantity >= quantity;
    }
};
}
