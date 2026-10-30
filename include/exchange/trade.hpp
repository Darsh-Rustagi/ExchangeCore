#pragma once

#include "exchange/types.hpp"
#include <string>

namespace exchange {
struct Trade {
    OrderId     buy_order_id{0};
    OrderId     sell_order_id{0};
    std::string symbol{};
    Price       price{0};
    Quantity    quantity{0};
    Timestamp   timestamp{0};
};
}
