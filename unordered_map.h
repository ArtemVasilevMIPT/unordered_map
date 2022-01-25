#pragma once

#include <iostream>
#include <vector>
#include <utility>
#include <memory>
#include <stdexcept>

template <typename Key, typename Value,
        typename Hash = std::hash<Key>,
        typename Equal = std::equal_to<Key>,
        typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap
{
public:
    using NodeType = std::pair<const Key, Value>;
private:

    struct Node
    {
        NodeType* value = nullptr;
        Node* next = nullptr;
        uint64_t cache = 0;

        template<typename T>
        Node(T&& pair) : value(std::forward<T>(pair)){}

        template<typename... Args>
        Node(Args&& ...args) : value(std::forward<Args>(args)...){}

        //Node(const NodeType& nt) : value(nt){}
        //Node(NodeType&& nt) : value(nt){}

    };
    using Allocator = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    using ATraits = std::allocator_traits<Allocator>;
    using Traits = std::allocator_traits<Alloc>;
    std::vector<Node*> table;
    Node* head = nullptr;

    Allocator a;
    Alloc al;
    Equal eq;
    Hash h;

    size_t bucketCount = 0;
    size_t maxBucketCount = 0;
    size_t elementCount = 0;

    double mlf = 0.75;

    size_t getHash(const Key& key)
    {
        return h(key) % maxBucketCount;
    }

    void rehash(size_t newCap)
    {
        std::vector<Node*> newTable(newCap, nullptr);
        maxBucketCount = newCap;
        Node* nd = head->next;
        bucketCount = 0;
        head->next = nullptr;
        while(nd != nullptr)
        {
            Node* buff = nd->next;
            size_t hash = getHash(nd->value->first);
            nd->cache = hash;
            if(newTable[hash])
            {
                newTable[hash]->next->next = nd->next;
                newTable[hash]->next->next = nd;
            }
            else
            {
                newTable[hash] = head;
                nd->next = head->next;
                head->next = nd;
                if(nd->next)
                {
                    newTable[nd->next->cache] = nd;
                }
                ++bucketCount;
            }
            nd = buff;
        }
        table = std::move(newTable);
    }

public:
    explicit UnorderedMap(const Hash& hs = Hash(), const Equal& e = Equal(), const Alloc& al = Alloc()) :
            eq(e), h(hs), elementCount(0)
    {
        a = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>(al);
        table.resize(64);
        maxBucketCount = 64;
        head = ATraits::allocate(a, 1);
        head->next = nullptr;
    }

    UnorderedMap(const UnorderedMap& other) :
            eq(other.eq),h(other.h), maxBucketCount(other.maxBucketCount),
            elementCount(other.elementCount), mlf(other.mlf)
    {
        a = ATraits::select_on_container_copy_construction(other.a);
        table.resize(maxBucketCount);
        head = ATraits::allocate(a, 1);
        head->next = nullptr;
        Node* nd = head;
        Node* curr = other.head->next;
        size_t curr_hash = -1;
        while(curr)
        {
            Node* newNode = ATraits::allocate(a, 1);
            ATraits::construct(a, newNode);
            NodeType* nt = Traits::allocate(al, 1);
            Traits::construct(al, nt, *(curr->value));
            newNode->value = nt;
            newNode->cache = curr->cache;
            newNode->next = nullptr;
            if (newNode->cache != curr_hash)
            {
                table[newNode->cache] = nd;
                curr_hash = newNode->cache;
            }
            nd->next = newNode;
            nd = nd->next;
            curr = curr->next;
        }
    }

    UnorderedMap(UnorderedMap&& other) noexcept :
            table(std::move(other.table)), head(other.head),
            eq(other.eq), h(other.h), maxBucketCount(other.maxBucketCount),
            elementCount(other.elementCount), mlf(other.mlf)
    {
        if(ATraits::propagate_on_container_move_assignment::value && a != other.a)
            a = other.a;
        other.head = ATraits::allocate(other.a, 1);
        other.head->next = nullptr;
        other.elementCount = 0;
    }

    ~UnorderedMap()
    {
        Node* nd = head;
        while(nd)
        {
            Node* buff = nd->next;
            if(nd != head)
                Traits::deallocate(al, nd->value, 1);
            ATraits::deallocate(a, nd, 1);
            nd = buff;
        }
    }

    void reserve(size_t newSize)
    {
        table.resize(newSize);
        rehash(newSize);
    }

    size_t max_size() const
    {
        return maxBucketCount;
    }

    double load_factor() const
    {
        return static_cast<double>(elementCount) / static_cast<double>(maxBucketCount);
    }

    double max_load_factor() const
    {
        return mlf;
    };
    void max_load_factor(double newLoadFactor)
    {
        size_t newSize = (mlf * maxBucketCount) / newLoadFactor;
        mlf = newLoadFactor;
        rehash(newSize);
    }

    UnorderedMap& operator=(const UnorderedMap& other)
    {
        if(this == &other)
            return *this;
        //Clearing
        Node* nd = head->next;
        while(nd)
        {
            Node* buff = nd->next;
            ATraits::deallocate(a, nd, 1);
            nd = buff;
        }
        //Copying
        if(ATraits::propagate_on_container_copy_assignment::value && a != other.a)
            a = other.a;
        elementCount = other.elementCount;
        maxBucketCount = other.maxBucketCount;
        eq = other.eq;
        h = other.h;
        mlf = other.mlf;
        table.resize(maxBucketCount);
        head = ATraits::allocate(a, 1);
        head->next = nullptr;
        head->cache = other.head->cache;
        nd = head;
        Node* curr = other.head->next;
        size_t curr_hash = -1;
        while(curr)
        {
            Node* newNode = ATraits::allocate(a, 1);
            ATraits::construct(a, newNode);
            NodeType* nt = Traits::allocate(al, 1);
            Traits::construct(al, nt, *(curr->value));
            newNode->value = nt;
            newNode->next = nullptr;
            if (newNode->cache != curr_hash)
            {
                table[newNode->cache] = nd;
                curr_hash = newNode->cache;
            }
            nd->next = newNode;
            nd = nd->Next;
            curr = curr->next;
        }
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& other) noexcept
    {
        if(this == &other)
            return *this;
        //Clearing
        Node* nd = head;
        while(nd)
        {
            Node* buff = nd->next;
            ATraits::deallocate(a, nd, 1);
            nd = buff;
        }
        //Moving
        table = std::move(other.table);
        head = other.head;
        if(ATraits::propagate_on_container_move_assignment::value && a != other.a)
            a = other.a;
        elementCount = other.elementCount;
        maxBucketCount = other.maxBucketCount;
        eq = other.eq;
        h = other.h;
        mlf = other.mlf;
        other.head = ATraits::allocate(other.a, 1);
        other.head->next = nullptr;
        other.elementCount = 0;
        return *this;
    }

    Value& operator[](const Key& key)
    {
        auto res = insert(NodeType(key, Value()));
        return res.first->second;
    }

    Value& at(const Key& key)
    {

        size_t hash = getHash(key);

        if(!table[hash])
        {
            throw std::out_of_range("No element with such key");
        }

        Node* nd = table[hash]->next;
        while(nd && nd->cache == hash)
        {
            if(eq(nd->value->first, key))
            {
                return nd->value->second;
            }
            nd = nd->next;
        }
        throw std::out_of_range("No element with such key");
    }
    const Value& at(const Key& key) const
    {
        size_t hash = getHash(key);

        if(!table[hash])
        {
            throw std::out_of_range("No element with such key");
        }

        Node* nd = table[hash]->next;
        while(nd && nd->cache == hash)
        {
            if(eq(nd->value.first, key))
            {
                return nd->value.second;
            }
            nd = nd->next;
        }
        throw std::out_of_range("No element with such key");
    }

    size_t size() const
    {
        return elementCount;
    }

    template<bool isConst>
    struct CommonIterator
    {
        friend UnorderedMap;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = NodeType;
        using pointer = std::conditional_t<isConst, const NodeType*, NodeType*>;
        using reference = std::conditional_t<isConst, const NodeType&, NodeType&>;
    private:
        Node* ptr;
        Node* next;
        CommonIterator(Node* nd) : ptr(nd)
        {
            if(ptr)
            {
                next = ptr->next;
            }
        }
    public:
        CommonIterator(const CommonIterator& other) : ptr(other.ptr), next(other.next){}
        CommonIterator& operator=(const CommonIterator& other)
        {
            ptr = other.ptr;
            next = other.next;
            return *this;
        }

        CommonIterator& operator++()
        {
            ptr = next;
            if(ptr)
            {
                next = ptr->next;
            }
            return *this;
        }
        CommonIterator operator++(int)
        {
            CommonIterator res(ptr);
            ptr = next;
            if(ptr)
            {
                next = ptr->next;
            }
            return res;
        }

        std::conditional_t<isConst, const NodeType&, NodeType&> operator*()
        {
            return *(ptr->value);
        }

        std::conditional_t<isConst, const NodeType*, NodeType*> operator->()
        {
            return ptr->value;
        }

        friend bool operator==(const CommonIterator& it1, const CommonIterator& it2)
        {
            return it1.ptr == it2.ptr;
        }
        friend bool operator!=(const CommonIterator& it1, const CommonIterator& it2)
        {
            return !(it1 == it2);
        }
    };

    using Iterator = CommonIterator<false>;
    using ConstIterator = CommonIterator<true>;

    Iterator begin()
    {
        return Iterator(head->next);
    }
    ConstIterator begin() const
    {
        return ConstIterator(head->next);
    }
    ConstIterator cbegin() const
    {
        return ConstIterator(head->next);
    }

    Iterator end()
    {
        return Iterator(nullptr);
    }
    ConstIterator end() const
    {
        return ConstIterator(nullptr);
    }
    ConstIterator cend() const
    {
        return ConstIterator(nullptr);
    }

    template<typename T>
    std::pair<Iterator, bool> insert(T&& nt)
    {
        if(load_factor() > max_load_factor())
        {
            rehash(maxBucketCount * 2);
        }
        auto it = find(nt.first);
        if(it.ptr == nullptr)
        {
            Node* newNode = ATraits::allocate(a, 1);
            ATraits::construct(a, newNode);
            NodeType* node = Traits::allocate(al, 1);
            Traits::construct(al, node, std::forward<T>(nt));
            newNode->value = node;
            size_t hash = getHash(newNode->value->first);
            newNode->cache = hash;
            if(!table[hash])
            {
                newNode->next = head->next;
                head->next = newNode;
                head->cache = hash;
                table[hash] = head;
                if(newNode->next)
                {
                    table[newNode->next->cache] = newNode;
                }
                ++elementCount;
                ++bucketCount;
                return std::make_pair(Iterator(newNode), true);
            }
            table[hash]->next->next = newNode->next;
            table[hash]->next->next = newNode;
            ++elementCount;
            return std::make_pair(Iterator(newNode), true);
        }
        return std::make_pair(it, false);
    }

    std::pair<Iterator, bool> insert(NodeType&& nt)
    {
        return insert<>(nt);
    }

    template <typename InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        for(auto it = first; it != last; ++it)
        {
            insert(*it);
        }
    }

    template <typename ...Args>
    std::pair<Iterator, bool> emplace(Args&&... args)
    {
        NodeType* nt = std::allocator_traits<Alloc>::allocate(al, 1);
        std::allocator_traits<Alloc>::construct(al, nt, std::forward<Args>(args)...);
        if(load_factor() > max_load_factor())
        {
            rehash(maxBucketCount * 2);
        }
        auto it = find(nt->first);
        if(it.ptr == nullptr)
        {
            Node* newNode = ATraits::allocate(a, 1);
            ATraits::construct(a, newNode);
            newNode->value = nt;
            size_t hash = getHash(newNode->value->first);
            newNode->cache = hash;
            if(!table[hash])
            {
                newNode->next = head->next;
                head->next = newNode;
                head->cache = hash;
                table[hash] = head;
                if(newNode->next)
                {
                    table[newNode->next->cache] = newNode;
                }
                ++elementCount;
                ++bucketCount;
                return std::make_pair(Iterator(newNode), true);
            }
            table[hash]->next->next = newNode->next;
            table[hash]->next->next = newNode;
            ++elementCount;
            return std::make_pair(Iterator(newNode), true);
        }
        std::allocator_traits<Alloc>::deallocate(al, nt, 1);
        return std::make_pair(it, false);
    }

    void erase(Iterator it)
    {
        Node* nd = it.ptr;
        //Finding previous element
        Node* prev = table[nd->cache];
        while(prev->next != nd)
        {
            prev = prev->next;
        }
        if(nd->next && nd->next->cache != nd->cache)
        {
            //Deletion of the last element in chain
            prev->next = nd->next;
            if(prev->cache != nd->cache)
            {
                --bucketCount;
                table[nd->cache] = nullptr;
            }
            table[nd->next->cache] = prev;
        }
        else if(!nd->next)
        {
            //Deletion of the last element in list
            prev->next = nullptr;
            if(prev->cache != nd->cache || prev == head)
            {
                table[nd->cache] = nullptr;
                --bucketCount;
            }
        }
        else
        {
            prev->next = nd->next;
        }
        --elementCount;
        Traits::deallocate(al, nd->value, 1);
        ATraits::deallocate(a, nd, 1);
    }
    void erase(Iterator first, Iterator last)
    {
        for(auto it = first; it != last; ++it)
        {
            erase(it);
        }
    }

    Iterator find(const Key& key)
    {
        size_t hash = getHash(key);

        Node* nd = table[hash];
        if(!nd)
        {
            return end();
        }
        nd = nd->next;
        while(nd && nd->cache == hash)
        {
            if(eq(nd->value->first, key))
            {
                return Iterator(nd);
            }
            nd = nd->next;
        }
        return end();
    }

    ConstIterator find(const Key& key) const
    {
        size_t hash = getHash(key);

        Node* nd = table[hash];
        if(!nd)
        {
            return cend();
        }
        nd = nd->next;
        while(nd && nd->cache == hash)
        {
            if(eq(nd->value.first, key))
            {
                return ConstIterator(nd);
            }
            nd = nd->next;
        }
        return cend();
    }
};
