#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() : m_queue(), m_mutex() {}
    void push(T&& value) {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_queue.push(std::forward<T>(value));
      m_condition.notify_one();
    }

    T pop() {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_condition.wait(lock, [this] { return !m_queue.empty(); });
      auto value = m_queue.front(); m_queue.pop();
      return value;
    }

private:
  std::queue<T> m_queue;
  std::mutex m_mutex;
  std::condition_variable m_condition;
};