#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto element = _lru_index.find(key);
    if (element == _lru_index.end()) {
        return putElement(key, value);
    }
    SimpleLRU::lru_node& node = element->second.get();
    return updateNode(node, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto element = _lru_index.find(key);
    if (element == _lru_index.end()) {
        return putElement(key, value);
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto element = _lru_index.find(key);
    if (element == _lru_index.end()) {
        return false;
    }
    SimpleLRU::lru_node& node = element->second.get();
    return updateNode(node, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto element = _lru_index.find(key);
    if (element == _lru_index.end()) {
        return false;
    }
    SimpleLRU::lru_node& node = element->second.get();

    deleteNode(node);

    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto element = _lru_index.find(key);
    if (element == _lru_index.end()) {
        return false;
    }

    SimpleLRU::lru_node& node = element->second.get();
    value = node.value;

    moveNode(node);

    return true;
}

void SimpleLRU::moveNode(SimpleLRU::lru_node& node) {
    if (node.next == nullptr) {
        return;
    }
    auto ptr = node.next->prev;
    ptr->next->prev = ptr->prev; // next ok

    if (ptr->prev == nullptr) {
        _lru_tail->next = std::move(_lru_head); // tail ok
        _lru_head = std::move(ptr->next); // head ok
    } else {
        _lru_tail->next = std::move(ptr->prev->next); // tail ok
        ptr->prev->next = std::move(ptr->next); // prev ok
    }

    ptr->next = nullptr;
    ptr->prev = _lru_tail;
    _lru_tail = ptr;
}

bool SimpleLRU::putElement(const std::string& key, const std::string& value) {
    std::size_t node_size = key.size() + value.size();
    if (node_size > _max_size) {
        return false;
    }
    while (_cur_size + node_size > _max_size) {
        deleteNode(std::ref(*_lru_head));
    }

    auto node = new lru_node {key, value, _lru_tail, nullptr};

    if (_lru_head == nullptr) {
        _lru_head = std::unique_ptr<lru_node>(node);
    } else {
        _lru_tail->next = std::unique_ptr<lru_node>(node);
    }

    _lru_tail = node;
    _cur_size += node_size;
    _lru_index.emplace(std::cref(_lru_tail->key), std::ref(*node));

    return true;
}

bool SimpleLRU::updateNode(SimpleLRU::lru_node& node, const std::string& value) {
    std::size_t new_value_size = value.size();
    std::size_t old_value_size = node.value.size();

    if (node.key.size() + new_value_size > _max_size) {
        return false;
    }

    moveNode(node);

    while (_cur_size - old_value_size + new_value_size > _max_size) {
        deleteNode(std::ref(*_lru_head));
    }

    node.value = value;
    _cur_size += new_value_size - old_value_size;

    return true;
}

void SimpleLRU::deleteNode(SimpleLRU::lru_node& node) {
    _cur_size -= (node.key.size() + node.value.size());
    _lru_index.erase(node.key);

    if (_lru_head.get() == _lru_tail) { // 1 element
        _lru_head = nullptr;
        _lru_tail = nullptr;
    } else if (node.prev == nullptr) { // head
        _lru_head = std::move(_lru_head->next);
        _lru_head->prev = nullptr;
    } else if (node.next == nullptr) {
        _lru_tail = _lru_tail->prev;
        _lru_tail->next = nullptr;
    } else {
        lru_node* prev_node = node.prev;
        prev_node->next = std::move(node.next);
        prev_node->next->prev = prev_node;
    }
}

} // namespace Backend
} // namespace Afina
