#include "./exchange/order_manager.hpp"

#include <algorithm>

namespace exchange {
Order *OrderManager::createOrder(Side side, OrderType type,
                                 std::string_view symbol, Price price,
                                 Quantity quantity, Timestamp timestamp,
                                 std::optional<OrderId> custom_id) {
  if (symbol.empty() || quantity == 0) {
    return nullptr;
  }

  if (type == OrderType::Limit && price <= 0) {
    return nullptr;
  }

  OrderId assigned_id = 0;
  if (custom_id.has_value()) {
    assigned_id = *custom_id;
    if (assigned_id >= next_id_) {
      next_id_ = assigned_id + 1;
    }
  } else {
    assigned_id = next_id_++;
  }

  if (orders_.find(assigned_id) != orders_.end()) {
    return nullptr;
  }

  if (timestamp == 0) {
    timestamp = currentNanos();
  }

  auto order = std::make_unique<Order>();
  order->id = assigned_id;
  order->symbol = std::string(symbol);
  order->side = side;
  order->type = type;
  order->price = (type == OrderType::Market) ? 0 : price;
  order->quantity = quantity;
  order->filled_quantity = 0;
  order->status = OrderStatus::New;
  order->timestamp = timestamp;

  Order *raw_ptr = order.get();
  orders_.emplace(assigned_id, std::move(order));
  return raw_ptr;
}

Order *OrderManager::getOrder(OrderId id) {
  auto it = orders_.find(id);
  if (it != orders_.end()) {
    return it->second.get();
  }
  return nullptr;
}

const Order *OrderManager::getOrder(OrderId id) const {
  auto it = orders_.find(id);
  if (it != orders_.end()) {
    return it->second.get();
  }
  return nullptr;
}

bool OrderManager::hasOrder(OrderId id) const noexcept {
  return orders_.find(id) != orders_.end();
}

std::size_t OrderManager::totalOrders() const noexcept {
  return orders_.size();
}

void OrderManager::clear() {
  orders_.clear();
  next_id_ = 1;
}
}
