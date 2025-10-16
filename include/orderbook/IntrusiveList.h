#pragma once

#include <cstddef>
#include <iterator>

#ifdef USE_BOOST_INTRUSIVE
#include <boost/intrusive/list.hpp>
#endif

namespace ob {

#ifndef USE_BOOST_INTRUSIVE

/**
 * @brief Minimal intrusive FIFO queue used to maintain price-time priority.
 *
 * @tparam Node Node type exposing `Node* next`, `Node* prev`, and `Order* order` members.
 */
template <typename Node>
class IntrusiveFifo {
public:
    IntrusiveFifo() = default;

    /// @return True when the queue holds no elements.
    bool empty() const noexcept { return head_ == nullptr; }

    /**
     * @brief Append a node to the end of the FIFO.
     * @param node Node inserted; must not already belong to this queue.
     */
    void push_back(Node* node) noexcept {
        node->next = nullptr;
        node->prev = tail_;
        if (tail_) {
            tail_->next = node;
        } else {
            head_ = node;
        }
        tail_ = node;
    }

    Node* front() noexcept { return head_; }
    const Node* front() const noexcept { return head_; }

    /// Remove the head element (if any) from the queue.
    void pop_front() noexcept {
        if (!head_) return;
        Node* next = head_->next;
        head_->next = nullptr;
        head_->prev = nullptr;
        head_ = next;
        if (!head_) tail_ = nullptr;
        else head_->prev = nullptr;
    }

    /**
     * @brief Remove an arbitrary node from the FIFO.
     * @param node Node to erase. No-op when nullptr.
     */
    void erase(Node* node) noexcept {
        if (!node) return;
        if (node == head_) {
            pop_front();
            return;
        }
        if (node == tail_) {
            tail_ = node->prev;
            if (tail_) tail_->next = nullptr;
            node->prev = nullptr;
            node->next = nullptr;
            return;
        }
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        node->next = nullptr;
        node->prev = nullptr;
    }

private:
    Node* head_{nullptr};
    Node* tail_{nullptr};
};

#else

/**
 * @brief Intrusive FIFO that delegates to `boost::intrusive::list` when enabled.
 *
 * @tparam Node Hook type deriving from `boost::intrusive::list_base_hook`.
 */
template <typename Node>
class IntrusiveFifo {
public:
    IntrusiveFifo() = default;

    /// @return True when the queue holds no elements.
    bool empty() const noexcept { return list_.empty(); }

    /**
     * @brief Append a node to the end of the FIFO.
     * @param node Node inserted; must not already belong to this queue.
     */
    void push_back(Node* node) noexcept {
        if (!node) return;
        list_.push_back(*node);
    }

    Node* front() noexcept {
        if (list_.empty()) return nullptr;
        return &list_.front();
    }

    const Node* front() const noexcept {
        if (list_.empty()) return nullptr;
        return &list_.front();
    }

    void pop_front() noexcept {
        if (list_.empty()) return;
        Node& node = list_.front();
        list_.pop_front();
        node.order = nullptr;
    }

    /**
     * @brief Remove an arbitrary node from the FIFO.
     * @param node Node to erase. No-op when nullptr.
     */
    void erase(Node* node) noexcept {
        if (!node) return;
        list_.erase(list_.iterator_to(*node));
        node->order = nullptr;
    }

private:
    using list_type = boost::intrusive::list<Node, boost::intrusive::constant_time_size<false>>;
    list_type list_{};
};

#endif

} // namespace ob
