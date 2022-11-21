/*
 * extendible_hash.h : implementation of in-memory hash table using extendible
 * hashing
 *
 * Functionality: The buffer pool manager must maintain a page table to be able
 * to quickly map a PageId to its corresponding memory location; or alternately
 * report that the PageId does not match any currently-buffered page.
 */

#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <memory>
#include "hash/hash_table.h"

using namespace std;

namespace scudb {

    template<typename K, typename V>
    class ExtendibleHash : public HashTable<K, V> {
        struct Bucket {
            Bucket(int depth) {
                localDepth = depth;
            }

            int localDepth;
            map<K, V> keymap;
            mutex latch;
        };

    public:
        // constructor 构造函数
        ExtendibleHash(size_t size);

        ExtendibleHash();

        // helper function to generate hash addressing
        size_t HashKey(const K &key) const;


        // helper function to get global & local depth
        int GetGlobalDepth() const;

        int GetLocalDepth(int bucket_id) const;

        int GetNumBuckets() const;

        // lookup and modifier
        int getIndex(const K &key) const;

        bool Find(const K &key, V &value) override;

        bool Remove(const K &key) override;

        void Insert(const K &key, const V &value) override;

    private:
        // add your own member variables here
        int globalDepth{};                    //桶全局深度
        size_t bucketSize{};                  //桶大小
        int bucketNum{};                      //桶的个数
        vector<shared_ptr<Bucket>> buckets;   //桶
        mutable mutex latch;                  //锁
    };
} // namespace scudb
