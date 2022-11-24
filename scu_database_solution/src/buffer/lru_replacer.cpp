/**
 * LRU implementation
 */
#include "buffer/lru_replacer.h"
#include "page/page.h"

namespace scudb {

    template<typename T>
    LRUReplacer<T>::LRUReplacer() {
        //make share 创建 shared ptr
        this->head = make_shared<Node>();
        this->tail = make_shared<Node>();
        this->head->next = tail;
        this->tail->prev = head;
    }

    template<typename T>
    LRUReplacer<T>::~LRUReplacer() {
        // 智能指针将会自己delete []
    }

/*
 * Insert value into LRU
 */
    template<typename T>
    void LRUReplacer<T>::Insert(const T &value) {
        //step1 上锁lru表锁
        std::lock_guard<mutex> lck(latch);
        //step2 添加一个节点指针 which 是指向value的
        std::shared_ptr<Node> curPtr;
        //step3 如果该页面存在map中
        if (map.find(value) != map.end()){
            // unordered_map.find()将会返回iterator,否则指向end
            //cur赋值为 key = value对应的指针，为value
            curPtr = map[value];
            std::shared_ptr<Node> prev = curPtr->prev;
            std::shared_ptr<Node> succ = curPtr->next;
            // 将 map[value]取出来，其前后节点相连
            prev->next = succ;
            succ->prev = prev;

        } else {
            curPtr = std::make_shared<Node>(value);
        }
        // 获得insert的节点，在header之后也就是second之前进行插入
        std::shared_ptr<Node> secPtr = head->next;
        curPtr->next = secPtr;
        secPtr->prev = curPtr;
        head->next = curPtr;
        curPtr->prev = head;
        map[value] = curPtr;
   }

/* If LRU is non-empty, pop the head member from LRU to argument "value", and
 * return true. If LRU is empty, return false
 */
    template<typename T>
    bool LRUReplacer<T>::Victim(T &value) {
        std::lock_guard<mutex> lck (latch);
        if (map.empty()){
        return false;
        }
        std::shared_ptr<Node> last = tail->prev;
        tail->prev = last->prev;
        last->prev->next = tail;
        value = last->val;
        map.erase(last->val);
        return true;
    }

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
    template<typename T>
    bool LRUReplacer<T>::Erase(const T &value) {
        std::lock_guard<mutex> lck(latch);
        if (map.find(value) != map.end()){
            std::shared_ptr<Node> cur = map[value];
            cur->prev->next = cur->next;
            cur->next->prev = cur->prev;
        }
        bool result = map.erase(value);
        return result;
    }

    template<typename T>
    size_t LRUReplacer<T>::Size() {
        std::lock_guard<mutex> lck (latch);
        return map.size();
    }

    template
    class LRUReplacer<Page *>;

// test only
    template
    class LRUReplacer<int>;

} // namespace scudb
