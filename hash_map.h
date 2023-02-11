#pragma once

#include <vector>
#include <list>
#include <iterator>

template<class KeyType, class ValueType, class Hasher = std::hash<KeyType>>
class HashMap {
    using PairType = std::pair<const KeyType, ValueType>;
    using BucketType = std::vector<std::list<PairType>>;

public:
    class iterator {
    public:
        iterator(BucketType* bucket, size_t i, typename std::list<PairType>::iterator it) : it_i_(i), it_capacity_(
                bucket->size()), it_(it), it_bucket_(bucket) {}

        iterator() = default;

        PairType& operator*() const {
            return *it_;
        }

        PairType* operator->() const {
            return &(*it_);
        }

        bool operator==(const iterator& other) const {
            return it_ == other.it_;
        }

        bool operator!=(const iterator& other) const {
            return it_ != other.it_;
        }

        iterator& operator++() {
            GetNext();
            return *this;
        }

        iterator operator++(int) {
            iterator new_it(*this);
            GetNext();
            return new_it;
        }

    private:
        friend class HashMap<KeyType, ValueType, Hasher>;

        size_t it_i_ = 0;
        size_t it_capacity_;
        typename std::list<PairType>::iterator it_;
        BucketType *it_bucket_;

        void GetNext() {
            if (it_ != (*it_bucket_)[it_i_].end()) {
                ++it_;
            }
            if (it_ == (*it_bucket_)[it_i_].end()) {
                do {
                    ++it_i_;
                } while (it_i_ < it_capacity_ && (*it_bucket_)[it_i_].empty());
                if (it_i_ < it_capacity_) {
                    it_ = (*it_bucket_)[it_i_].begin();
                } else {
                    it_ = (*it_bucket_)[it_capacity_ - 1].end();
                }
            }
        }
    };

    class const_iterator {
    public:
        const_iterator(const BucketType* bucket, size_t i, typename std::list<PairType>::const_iterator it)
                : it_i_(i), it_capacity_(bucket->size()), it_(it), it_bucket_(bucket) {}

        const_iterator() = default;

        const PairType& operator*() const {
            return *it_;
        }

        const PairType* operator->() const {
            return &(*it_);
        }

        bool operator==(const const_iterator& other) const {
            return it_ == other.it_;
        }

        bool operator!=(const const_iterator& other) const {
            return it_ != other.it_;
        }

        const_iterator& operator++() {
            GetNext();
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator new_it(*this);
            GetNext();
            return new_it;
        }

    private:
        friend class HashMap<KeyType, ValueType, Hasher>;

        size_t it_i_ = 0;
        size_t it_capacity_;
        typename std::list<PairType>::const_iterator it_;
        const BucketType *it_bucket_;

        void GetNext() {
            if (it_ != (*it_bucket_)[it_i_].end()) {
                ++it_;
            }
            if (it_ == (*it_bucket_)[it_i_].end()) {
                do {
                    ++it_i_;
                } while (it_i_ < it_capacity_ && (*it_bucket_)[it_i_].empty());
                if (it_i_ < it_capacity_) {
                    it_ = (*it_bucket_)[it_i_].begin();
                } else {
                    it_ = (*it_bucket_)[it_capacity_ - 1].end();
                }
            }
        }
    };

    iterator begin() {
        for (size_t i = 0; i < capacity_; ++i) {
            if (!bucket_[i].empty()) {
                return iterator(&bucket_, i, bucket_[i].begin());
            }
        }
        return iterator(&bucket_, capacity_ - 1, bucket_[capacity_ - 1].end());
    }

    iterator end() {
        return iterator(&bucket_, capacity_ - 1, bucket_[capacity_ - 1].end());
    }

    iterator find(KeyType key) {
        for (size_t p : primes_) {
            size_t hash = GetHash(key, capacity_, p);
            for (auto it = bucket_[hash].begin(); it != bucket_[hash].end(); ++it) {
                if (it->first == key) {
                    return iterator(&bucket_, hash, it);
                }
            }
        }

        return iterator(&bucket_, capacity_ - 1, bucket_[capacity_ - 1].end());
    }

    const_iterator begin() const {
        for (size_t i = 0; i < capacity_; ++i) {
            if (!bucket_[i].empty()) {
                return const_iterator(&bucket_, i, bucket_[i].begin());
            }
        }
        return const_iterator(&bucket_, capacity_ - 1, bucket_[capacity_ - 1].end());
    }

    const_iterator end() const {
        return const_iterator(&bucket_, capacity_ - 1, bucket_[capacity_ - 1].end());
    }

    const_iterator find(KeyType key) const {
        for (size_t p : primes_) {
            size_t hash = GetHash(key, capacity_, p);
            for (auto it = bucket_[hash].begin(); it != bucket_[hash].end(); ++it) {
                if (it->first == key) {
                    return const_iterator(&bucket_, hash, it);
                }
            }
        }

        return const_iterator(&bucket_, capacity_ - 1, bucket_[capacity_ - 1].end());
    }

    HashMap() : hasher_() {
        BuildEmptyHashMap();
    }

    HashMap(Hasher hasher) : hasher_(hasher) {
        BuildEmptyHashMap();
    }

    HashMap(std::initializer_list<PairType> list) : hasher_() {
        BuildWithInitializerList(list);
    }

    HashMap(std::initializer_list<PairType> list, Hasher hasher) : hasher_(hasher) {
        BuildWithInitializerList(list);
    }

    template<typename IteratorType>
    HashMap(IteratorType begin, IteratorType end) : hasher_() {
        BuildWithIterators(begin, end);
    }

    template<typename IteratorType>
    HashMap(IteratorType begin, IteratorType end, Hasher hasher) : hasher_(hasher_) {
        BuildWithIterators(begin, end);
    }

    HashMap& operator=(const HashMap<KeyType, ValueType, Hasher>& other) {
        if (this == &other) {
            return *this;
        }

        CopyHashMap(other);
        return *this;
    }

    HashMap(const HashMap<KeyType, ValueType, Hasher>& other) {
        CopyHashMap(other);
    }

    Hasher hash_function() const {
        return hasher_;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    void clear() {
        BuildEmptyHashMap();
    }

    void insert(const PairType pair) {
        RebuildIfNeed();

        size_t min_hash = 0;
        size_t min_size = kInfinity;
        const auto &[key, value] = pair;

        for (size_t p : primes_) {
            size_t hash = GetHash(key, capacity_, p);
            auto it = Find(key, hash);
            if (it != bucket_[hash].end()) {
                return;
            }

            size_t cur_size = bucket_[hash].size();
            if (cur_size < min_size) {
                min_hash = hash;
                min_size = cur_size;
            }
        }

        ++size_;
        bucket_[min_hash].push_front(pair);
        if (size_ == capacity_) {
            need_rebuild_ = true;
        }
    }

    void erase(const KeyType key) {
        for (size_t p : primes_) {
            size_t hash = GetHash(key, capacity_, p);
            auto it = Find(key, hash);

            if (it != bucket_[hash].end()) {
                --size_;
                bucket_[hash].erase(it);
                return;
            }
        }
    }

    ValueType& operator[](const KeyType key) {
        RebuildIfNeed();

        size_t min_hash = 0;
        size_t min_size = kInfinity;
        for (size_t p : primes_) {
            size_t hash = GetHash(key, capacity_, p);
            auto it = Find(key, hash);
            if (it != bucket_[hash].end()) {
                return it->second;
            }

            size_t cur_size = bucket_[hash].size();
            if (cur_size < min_size) {
                min_hash = hash;
                min_size = cur_size;
            }
        }

        std::pair<KeyType, ValueType> pair;
        pair.first = key;
        bucket_[min_hash].push_front(pair);
        ++size_;

        if (size_ == capacity_) {
            need_rebuild_ = true;
        }

        return bucket_[min_hash].begin()->second;
    }

    const ValueType& at(const KeyType key) const {
        for (size_t p : primes_) {
            size_t hash = GetHash(key, capacity_, p);
            auto it = FindConst(key, hash);

            if (it != bucket_[hash].end()) {
                return it->second;
            }
        }

        throw std::out_of_range("std::out_of_range");
    };

private:
    const size_t kDefaultCapacity = 4;
    const size_t kInfinity = 1e12;

    size_t size_ = 0;
    size_t capacity_ = kDefaultCapacity;
    const std::vector<size_t> primes_ = {1, 2, 3, 5};

    bool need_rebuild_ = false;

    BucketType bucket_;
    Hasher hasher_;

    size_t GetHash(const KeyType& key, const size_t capacity, const size_t k) const {
        return hasher_(key) * k % capacity;
    }

    void BuildEmptyHashMap(size_t capacity = 4) {
        size_ = 0;
        capacity_ = capacity;
        need_rebuild_ = false;

        bucket_.clear();
        bucket_.resize(capacity_);
    }

    void BuildWithInitializerList(std::initializer_list<PairType>& list) {
        BuildEmptyHashMap();
        for (auto it : list) {
            insert({it.first, it.second});
        }
    }

    template<typename IteratorType>
    void BuildWithIterators(IteratorType begin, IteratorType end) {
        BuildEmptyHashMap();
        for (auto it = begin; it != end; ++it) {
            insert({it->first, it->second});
        }
    }

    void CopyHashMap(const HashMap<KeyType, ValueType, Hasher>& other) {
        BuildEmptyHashMap(other.capacity_);
        hasher_ = other.hasher_;
        for (size_t i = 0; i < capacity_; ++i) {
            for (auto it : other.bucket_[i]) {
                bucket_[i].push_front(it);
            }
        }
    }

    void RebuildIfNeed() {
        if (need_rebuild_ == false) {
            return;
        }

        need_rebuild_ = false;
        size_t new_capacity = capacity_ * 2;
        BucketType new_bucket(new_capacity);

        for (size_t i = 0; i < capacity_; ++i) {
            for (auto it = bucket_[i].begin(); it != bucket_[i].end(); ++it) {
                size_t min_hash = 0;
                size_t min_size = kInfinity;
                for (size_t p : primes_) {
                    size_t hash = GetHash(it->first, new_capacity, p);
                    size_t cur_size = new_bucket[hash].size();
                    if (cur_size < min_size) {
                        min_hash = hash;
                        min_size = cur_size;
                    }
                }
                new_bucket[min_hash].push_front(*it);
            }
        }

        capacity_ = new_capacity;
        std::swap(bucket_, new_bucket);
    }

    typename std::list<PairType>::iterator Find(const KeyType& key, const size_t hash) {
        for (auto it = bucket_[hash].begin(); it != bucket_[hash].end(); ++it) {
            if (it->first == key) {
                return it;
            }
        }
        return bucket_[hash].end();
    }

    typename std::list<PairType>::const_iterator FindConst(const KeyType& key, const size_t hash) const {
        for (auto it = bucket_[hash].begin(); it != bucket_[hash].end(); ++it) {
            if (it->first == key) {
                return it;
            }
        }
        return bucket_[hash].end();
    }
};