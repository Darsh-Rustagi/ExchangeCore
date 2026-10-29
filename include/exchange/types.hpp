#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <chrono>

namespace exchange {
using OrderId   = std::uint64_t;
using Price     = std::int64_t;
using Quantity  = std::uint32_t;
using Timestamp = std::uint64_t;

enum class Side : std::uint8_t {
    Buy,
    Sell
};

enum class OrderType : std::uint8_t {
    Limit,
    Market
};

enum class OrderStatus : std::uint8_t {
    New,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected
};

inline constexpr std::string_view toString(Side side) noexcept {
    switch (side) {
        case Side::Buy:  return "Buy";
        case Side::Sell: return "Sell";
    }
    return "Unknown";
}

inline constexpr std::string_view toString(OrderType type) noexcept {
    switch (type) {
        case OrderType::Limit:  return "Limit";
        case OrderType::Market: return "Market";
    }
    return "Unknown";
}

inline constexpr std::string_view toString(OrderStatus status) noexcept {
    switch (status) {
        case OrderStatus::New:             return "New";
        case OrderStatus::PartiallyFilled: return "PartiallyFilled";
        case OrderStatus::Filled:          return "Filled";
        case OrderStatus::Cancelled:       return "Cancelled";
        case OrderStatus::Rejected:        return "Rejected";
    }
    return "Unknown";
}

inline Side sideFromString(std::string_view sv) {
    if (sv == "Buy" || sv == "BUY" || sv == "buy" || sv == "B" || sv == "b") {
        return Side::Buy;
    }
    return Side::Sell;
}

inline OrderType orderTypeFromString(std::string_view sv) {
    if (sv == "Market" || sv == "MARKET" || sv == "market" || sv == "M" || sv == "m") {
        return OrderType::Market;
    }
    return OrderType::Limit;
}

inline Timestamp currentNanos() noexcept {
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
}
}
