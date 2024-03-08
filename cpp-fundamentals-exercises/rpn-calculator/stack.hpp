#include <array>
#include <cstddef>
#include <deque>
#include <optional>

// Primary template.
template<typename Type, std::size_t MaxSize>
class Stack {
public:
  bool push(Type&& element);
  [[nodiscard]] std::optional<Type> pop();

  [[nodiscard]] std::size_t size() const noexcept;

private:
  std::deque<Type> data_;
};

template<typename Type, std::size_t MaxSize>
bool Stack<Type, MaxSize>::push(Type&& element) {
  if (data_.size() < MaxSize) {
    data_.emplace_back(std::move(element));
    return true;
  }

  return false;
}

template<typename Type, std::size_t MaxSize>
std::optional<Type> Stack<Type, MaxSize>::pop() {
  std::optional<Type> result;

  if (data_.empty()) {
    return result;
  }

  result = std::move(data_.back());
  data_.pop_back();

  return result;
}

template<typename Type, std::size_t MaxSize>
std::size_t Stack<Type, MaxSize>::size() const noexcept {
  return data_.size();
}

// Partial specialization that specializes on MaxSize = 2.
template<typename Type>
class Stack<Type, 2> {
public:
  bool push(Type&& element);
  [[nodiscard]] std::optional<Type> pop();

  [[nodiscard]] std::size_t size() const noexcept;

private:
  std::array<Type, 2> data_;
  std::size_t sp_{};
};

template<typename Type>
bool Stack<Type, 2>::push(Type&& element) {
  if (sp_ < 2) {
    data_.at(sp_++) = std::move(element);
    return true;
  }

  return false;
}

template<typename Type>
std::optional<Type> Stack<Type, 2>::pop() {
  std::optional<Type> result;

  if (sp_ > 0) {
    result = std::move(data_.at(--sp_));
  }

  return result;
}

template<typename Type>
std::size_t Stack<Type, 2>::size() const noexcept {
  return sp_;
}
