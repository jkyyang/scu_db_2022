/**
 * b_plus_tree_leaf_page.cpp
 * final Feb 1
 */

#include <sstream>
#include <include/page/b_plus_tree_internal_page.h>

#include "common/exception.h"
#include "common/rid.h"
#include "page/b_plus_tree_leaf_page.h"

namespace scudb {

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set page id/parent id, set
 * next page id and set max size
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(page_id_t page_id, page_id_t parent_id) {
        // init all the staff
        this->SetPageType(IndexPageType::LEAF_PAGE);
        this->SetSize(0);
        this->SetPageId(page_id);
        this->SetParentPageId(parent_id);
        this->SetNextPageId(INVALID_PAGE_ID);
        //  with first invalid
        int size = (PAGE_SIZE - sizeof(BPlusTreePage)) / sizeof(MappingType) - 1;
        this->SetMaxSize(size);
    }


/**
 * Helper methods to set/get next page id
 */
    INDEX_TEMPLATE_ARGUMENTS
    page_id_t B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const {
        return next_page_id_;
    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) {
        this->next_page_id_ = next_page_id;
    }

/**
 * Helper method to find the first index i so that array[i].first >= key
 * NOTE: This method is only used when generating index iterator
 */
    INDEX_TEMPLATE_ARGUMENTS
    int B_PLUS_TREE_LEAF_PAGE_TYPE::KeyIndex(
            const KeyType &key, const KeyComparator &comparator) const {

        // if there's no ptr
        assert(this->GetSize() >= 0);
        // else ok
        int leftPos = 0, rightPos = this->GetSize() - 1;
        while (leftPos <= rightPos) {
            int midPos = (leftPos + rightPos) / 2;
            if (comparator(array[midPos].first, key) >= 0) {
                rightPos = midPos - 1;
            } else {
                leftPos = midPos + 1;
            }
        }
        return rightPos + 1;
    }


/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
    INDEX_TEMPLATE_ARGUMENTS
    KeyType B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const {
        return array[index].first;
    }


/*
 * Helper method to find and return the key & value pair associated with input
 * "index"(a.k.a array offset)
 */
    INDEX_TEMPLATE_ARGUMENTS
    const MappingType &B_PLUS_TREE_LEAF_PAGE_TYPE::GetItem(int index) {
        return array[index];
    }


/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert key & value pair into leaf page ordered by key
 * @return  page size after insertion
 */
    INDEX_TEMPLATE_ARGUMENTS
    int B_PLUS_TREE_LEAF_PAGE_TYPE::Insert(const KeyType &key,
                                           const ValueType &value,
                                           const KeyComparator &comparator) {

        // leaf page
        int curIndex = this->KeyIndex(key, comparator);
        this->IncreaseSize(1);

        int curSize = this->GetSize();
        for (int i = curSize - 1; i > curIndex; i--) {
            this->array[i].first = this->array[i - 1].first;
            this->array[i].second = this->array[i - 1].second;
        }

        this->array[curIndex].first = key;
        this->array[curIndex].second = value;
        return curSize;


    }


/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveHalfTo(
            BPlusTreeLeafPage *recipient,
            __attribute__((unused)) BufferPoolManager *buffer_pool_manager) {
        // 考虑奇数 1 2 3 4 => 1 1 2 2,
        int recipient_index = (this->GetMaxSize() + 1) / 2;

        // copy the pairs from old page
        for (int i = recipient_index; i <= this->GetMaxSize(); ++i) {
            recipient->array[i - recipient_index].first = this->array[i].first;
            recipient->array[i - recipient_index].second = this->array[i].second;
        }

        //set pointer
        // 指针添加
        recipient->SetNextPageId(this->GetNextPageId());
        this->SetNextPageId(recipient->GetPageId());

        //set size
        this->SetSize(recipient_index);
        recipient->SetSize(recipient_index);


    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyHalfFrom(MappingType *items, int size) {


    }

/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * For the given key, check to see whether it exists in the leaf page. If it
 * does, then store its corresponding value in input "value" and return true.
 * If the key does not exist, then return false
 */
    INDEX_TEMPLATE_ARGUMENTS
    bool B_PLUS_TREE_LEAF_PAGE_TYPE::Lookup(const KeyType &key, ValueType &value,
                                            const KeyComparator &comparator) const {
        int index = this->KeyIndex(key, comparator);
        if (index < GetSize() && comparator(array[index].first, key) == 0) {
            value = array[index].second;
            return true;
        }
        return false;
    }

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * First look through leaf page to see whether delete key exist or not. If
 * exist, perform deletion, otherwise return immdiately.
 * NOTE: store key&value pair continuously after deletion
 * @return   page size after deletion
 */
    INDEX_TEMPLATE_ARGUMENTS
    int B_PLUS_TREE_LEAF_PAGE_TYPE::RemoveAndDeleteRecord(
            const KeyType &key, const KeyComparator &comparator) {

        // look through leaf page to see whether delete key exist or not.

        int curKeyIndex = this->KeyIndex(key, comparator);
        int curSize = this->GetSize();
        if (curKeyIndex >= curSize || comparator(key, KeyAt(curKeyIndex)) != 0) {
            // 如果越界 or 不相等
            return curSize;
        }

        // quick deletion
        int tarIdx = curKeyIndex;
        memmove((void *) (array + tarIdx), array + tarIdx + 1,
                static_cast<size_t>((GetSize() - tarIdx - 1) * sizeof(MappingType)));
        IncreaseSize(-1);
        return this->GetSize();
    }

/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all of key & value pairs from this page to "recipient" page, then
 * update next page id
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveAllTo(BPlusTreeLeafPage *recipient,
                                               int, BufferPoolManager *) {

        assert(recipient != nullptr);
        // we have copied the previous half, so just copy the last half
        // the index is started by zero, so the sizeNumber is the next position of right pos
        int index = recipient->GetSize();
        for (int i = 0; i < this->GetSize(); ++i, ++index) {
            recipient->array[index].first = this->array[i].first;
            recipient->array[index].second = this->array[i].second;
        }
        // set pointer like MoveHalfTo(),连接
        recipient->SetNextPageId(this->GetNextPageId());
        // sert size
        recipient->IncreaseSize(this->GetSize());
        this->SetSize(0);

    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyAllFrom(MappingType *items, int size) {}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveFirstToEndOf(
            BPlusTreeLeafPage *recipient,
            BufferPoolManager *buffer_pool_manager) {




        MappingType curPair = this->GetItem(0);
        // 取出
        this->IncreaseSize(-1);
        memmove(array, array + 1, static_cast<size_t>(GetSize() * sizeof(MappingType)));

        recipient->CopyLastFrom(curPair);
        //update relevant key & value curPair in its parent page.

        Page *page = buffer_pool_manager->FetchPage(GetParentPageId());
        B_PLUS_TREE_INTERNAL_PAGE *parent = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE *>(page->GetData());
        parent->SetKeyAt(parent->ValueIndex(GetPageId()), array[0].first);
        buffer_pool_manager->UnpinPage(GetParentPageId(), true);
    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyLastFrom(const MappingType &item) {

        assert(GetSize() + 1 <= GetMaxSize());
        array[GetSize()] = item;
        IncreaseSize(1);
    }
/*
 * Remove the last key & value pair from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveLastToFrontOf(
            BPlusTreeLeafPage *recipient, int parentIndex,
            BufferPoolManager *buffer_pool_manager) {

        MappingType pair = GetItem(GetSize() - 1);
        IncreaseSize(-1);
        recipient->CopyFirstFrom(pair, parentIndex, buffer_pool_manager);
    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyFirstFrom(
            const MappingType &item, int parentIndex,
            BufferPoolManager *buffer_pool_manager) {

        assert(GetSize() + 1 < GetMaxSize());
        memmove((void *) (array + 1), array, GetSize() * sizeof(MappingType));
        IncreaseSize(1);
        array[0] = item;

        Page *page = buffer_pool_manager->FetchPage(GetParentPageId());
        B_PLUS_TREE_INTERNAL_PAGE *parent = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE *>(page->GetData());
        parent->SetKeyAt(parentIndex, array[0].first);
        buffer_pool_manager->UnpinPage(GetParentPageId(), true);

    }

/*****************************************************************************
 * DEBUG
 *****************************************************************************/
    INDEX_TEMPLATE_ARGUMENTS
    std::string B_PLUS_TREE_LEAF_PAGE_TYPE::ToString(bool verbose) const {
        if (GetSize() == 0) {
            return "";
        }
        std::ostringstream stream;
        if (verbose) {
            stream << "[pageId: " << GetPageId() << " parentId: " << GetParentPageId()
                   << "]<" << GetSize() << "> ";
        }
        int entry = 0;
        int end = GetSize();
        bool first = true;

        while (entry < end) {
            if (first) {
                first = false;
            } else {
                stream << " ";
            }
            stream << std::dec << array[entry].first;
            if (verbose) {
                stream << "(" << array[entry].second << ")";
            }
            ++entry;
        }
        return stream.str();
    }

    template
    class BPlusTreeLeafPage<GenericKey<4>, RID, GenericComparator<4>>;

    template
    class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>>;

    template
    class BPlusTreeLeafPage<GenericKey<16>, RID, GenericComparator<16>>;

    template
    class BPlusTreeLeafPage<GenericKey<32>, RID, GenericComparator<32>>;

    template
    class BPlusTreeLeafPage<GenericKey<64>, RID, GenericComparator<64>>;
} // namespace scudb


