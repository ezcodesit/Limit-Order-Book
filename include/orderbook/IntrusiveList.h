#pragma once

#include <cstddef>
#include <iterator>

namespace ob {

template <typename Node>
class IntrusiveFifo {
public:
    IntrusiveFifo() = default;

    bool empty() const noexcept { return head_ == nullptr; }

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

    void pop_front() noexcept {
        if (!head_) return;
        Node* next = head_->next;
        head_->next = nullptr;
        head_->prev = nullptr;
        head_ = next;
        if (!head_) tail_ = nullptr;
        else head_->prev = nullptr;
    }

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

} // namespace ob
