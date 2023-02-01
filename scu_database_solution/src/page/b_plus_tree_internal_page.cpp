/**
 * b_plus_tree_internal_page.cpp
 */
#include <iostream>
#include <sstream>

#include "common/exception.h"
#include "page/b_plus_tree_internal_page.h"

namespace scudb {
/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/
/*
 * Init method after creating a new internal page
 * Including set page type, set current size, set page id, set parent id and set
 * max page size
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(page_id_t page_id,
                                              page_id_t parent_id) {
        //set all the parameters
        // set page type
        this->SetPageType(IndexPageType::INTERNAL_PAGE);
        // current size
        this->SetSize(0);
        // page id
        this->SetPageId(page_id);
        // parent id
        this->SetParentPageId(parent_id);
        // max page size
        int size = (PAGE_SIZE - sizeof(BPlusTreeInternalPage)) / sizeof(MappingType) - 1;
        //      except for the the first invalid key
        this->SetMaxSize(size);
    }
/*
 * Helper method to get/set the key associated with input "index"(a.k.a
 * array offset)
 */
    INDEX_TEMPLATE_ARGUMENTS
    KeyType B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const {
        // if out of the border, assert it immediately
        assert (index >= 0 && index < this->GetSize());
        // else return the key of pair
        return array[index].first;
    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) {
        // if out of the border, assert it immediately
        assert (index >= 0 && index < this->GetSize());
        // else change the key of pair
        array[index].first = key;
    }

/*
 * Helper method to find and return array index(or offset), so that its value
 * equals to input "value"
 */
    INDEX_TEMPLATE_ARGUMENTS
    int B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueIndex(const ValueType &value) const {
        // traversal all the value via array index/ offset
        int size = this->GetSize();
        for (int offset_ = 0; offset_ < size; ++offset_) {
            if (value != this->ValueAt(offset_)) {
                continue;
            }
            return offset_;
        }
        // fail to search the index of specific value
        return -1;

    }

/*
 * Helper method to get the value associated with input "index"(a.k.a array
 * offset)
 */
    INDEX_TEMPLATE_ARGUMENTS
    ValueType B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const {
        // test if out of the border via assert
        assert(index >= 0 && index < this->GetSize());
        ValueType value = this->array[index].second;
        return value;
    }

/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * Find and return the child pointer(page_id) which points to the child page
 * that contains input "key"
 * Start the search from the second key(the first key should always be invalid)
 */
    INDEX_TEMPLATE_ARGUMENTS
    ValueType
    B_PLUS_TREE_INTERNAL_PAGE_TYPE::Lookup(const KeyType &key,
                                           const KeyComparator &comparator) const {

        // if size > 1, then we can look up the page_id
        // because the first key should always be invalid
        assert(this->GetSize() >= 2);

        //start binary search

        int leftPos = 1, rightPos = this->GetSize() - 1;
        while (leftPos <= rightPos) {
            int midPos = (leftPos + rightPos) / 2;
            if (comparator(array[midPos].first, key) <= 0) {
                // the result of comparator <=0 implies that the aim is on the right of mid pos
                // fresh the left and mid
                leftPos = midPos +1;
            } else {
                rightPos = midPos - 1;
            }
        }
        return this->array[leftPos-1].second;
    }

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Populate new root page with old_value + new_key & new_value
 * When the insertion cause overflow from leaf page all the way upto the root
 * page, you should create a new root page and populate its elements.
 * NOTE: This method is only called within InsertIntoParent()(b_plus_tree.cpp)
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::PopulateNewRoot(
            const ValueType &old_value, const KeyType &new_key,
            const ValueType &new_value) {



        this->array[0].second = old_value;
        array[1].first = new_key;
        array[1].second = new_value;
        // set new size
        this->SetSize(2);
    }
/*
 * Insert new_key & new_value pair right after the pair with its value ==
 * old_value
 * @return:  new size after insertion
 */
    INDEX_TEMPLATE_ARGUMENTS
    int B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertNodeAfter(
            const ValueType &old_value, const KeyType &new_key,
            const ValueType &new_value) {
        int index = this->ValueIndex(old_value) +1;
        // valueIndex+1 should > 0 because error-1
        // test if is ok via assert
        assert(index>0);

        // start insert new nodd
        //  step 1 size++
        this->IncreaseSize(1);
        //  step 2 move the pairs from left to right
        int curSize = this->GetSize();

        for (int pos = curSize -1; pos > index; --pos) {
            array[pos].first = array[pos-1].first;
            array[pos].second = array[pos-1].second;
        }
        //  step 3 fresh the index position node using input parameters
        this->array[index].first = new_key;
        this->array[index].second = new_value;

        return curSize;
    }

/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveHalfTo(
            BPlusTreeInternalPage *recipient,
            BufferPoolManager *buffer_pool_manager) {



        assert(recipient != nullptr);
        int total = GetMaxSize() + 1;
        assert(GetSize() == total);
        //copy last half


        int recipient_index = (this->GetMaxSize() + 1) / 2;

        page_id_t recipient_pageId = recipient->GetPageId();



        for (int i = recipient_index; i <= this->GetMaxSize() ; i++) {

            recipient->array[i - recipient_index].first = array[i].first;
            recipient->array[i - recipient_index].second = array[i].second;


            //update children's parent page


            auto childRawPage = buffer_pool_manager->FetchPage(array[i].second);
            BPlusTreePage *childTreePage = reinterpret_cast<BPlusTreePage *>(childRawPage->GetData());
            childTreePage->SetParentPageId(recipient_pageId);
            buffer_pool_manager->UnpinPage(array[i].second,true);

        }

        //set size
        this->SetSize(recipient_index);
        recipient->SetSize(total - recipient_index);
    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyHalfFrom(
            MappingType *items, int size, BufferPoolManager *buffer_pool_manager) {


    }
/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Remove the key & value pair in internal page according to input index(a.k.a
 * array offset)
 * NOTE: store key&value pair continuously after deletion
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Remove(int index) {
        // test if out of the border via assert
        assert(index >= 0 && index < this->GetSize());
        // delete it from right to left
        for (int i = index+1; i < this->GetSize(); ++i) {
            //cover the pair[index] <<
            array[i-1] = array[i];
        }
        // decrease 1 in size
        IncreaseSize(-1);
    }

/*
 * Remove the only key & value pair in internal page and return the value
 * NOTE: only call this method within AdjustRoot()(in b_plus_tree.cpp)
 */
    INDEX_TEMPLATE_ARGUMENTS
    ValueType B_PLUS_TREE_INTERNAL_PAGE_TYPE::RemoveAndReturnOnlyChild() {
        // first pair is head
        ValueType tmp = this->ValueAt(0);
        // decrease 1 size
        this->IncreaseSize(-1);
        // what if the size == 0
        assert(this->GetSize() == 0);
        return tmp;
    }
/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all of key & value pairs from this page to "recipient" page, then
 * update relavent key & value pair in its parent page.
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveAllTo(
            BPlusTreeInternalPage *recipient, int index_in_parent,
            BufferPoolManager *buffer_pool_manager) {
//        // task 1
//        // Remove all of key & value pairs from this page to "recipient" page
//
//        // recipient parameter
//        int rec_size = recipient->GetSize();
//        page_id_t rec_PageId = recipient->GetPageId();
//        // find current parent
//        // parent page ptr
//        auto page = buffer_pool_manager->FetchPage(this->GetParentPageId());
//        assert(page != nullptr);
//        auto parent = reinterpret_cast<BPlusTreeInternalPage *>(page->GetPageId());
        int start = recipient->GetSize();
        page_id_t recipPageId = recipient->GetPageId();
        // first find parent
        Page *page = buffer_pool_manager->FetchPage(GetParentPageId());
        assert(page != nullptr);
        BPlusTreeInternalPage *parent = reinterpret_cast<BPlusTreeInternalPage *>(page->GetData());


        // the separation key from parent
        this->SetKeyAt(0, parent->KeyAt(index_in_parent));
        buffer_pool_manager->UnpinPage(parent->GetPageId(), false);
        for (int i = 0; i < GetSize(); ++i) {
            recipient->array[start + i].first = array[i].first;
            recipient->array[start + i].second = array[i].second;
            //update children's parent page
            auto childRawPage = buffer_pool_manager->FetchPage(array[i].second);
            BPlusTreePage *childTreePage = reinterpret_cast<BPlusTreePage *>(childRawPage->GetData());
            childTreePage->SetParentPageId(recipPageId);
            buffer_pool_manager->UnpinPage(array[i].second,true);
        }
        // task 2
        // Update relevant key & value pair in its parent page.
        recipient->SetSize(start + GetSize());
        assert(recipient->GetSize() <= GetMaxSize());
        this->SetSize(0);



    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyAllFrom(
            MappingType *items, int size, BufferPoolManager *buffer_pool_manager) {

    }

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to tail of "recipient"
 * page, then update relavent key & value pair in its parent page.
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveFirstToEndOf(
            BPlusTreeInternalPage *recipient,
            BufferPoolManager *buffer_pool_manager) {


        MappingType curPair{
            this->KeyAt(0),
            this->ValueAt(0)
        };
        this->IncreaseSize(-1);

        memmove(array, array + 1, static_cast<size_t>(GetSize()*sizeof(MappingType)));
        recipient->CopyLastFrom(curPair, buffer_pool_manager);



        // update child parent page id
        page_id_t childPageId = curPair.second;
        Page *page = buffer_pool_manager->FetchPage(childPageId);
        assert (page != nullptr);

        BPlusTreePage *child = reinterpret_cast<BPlusTreePage *>(page->GetData());
        child->SetParentPageId(recipient->GetPageId());
        assert(child->GetParentPageId() == recipient->GetPageId());
        buffer_pool_manager->UnpinPage(child->GetPageId(), true);


        //update relevant key & value curPair in its parent page.

        page = buffer_pool_manager->FetchPage(GetParentPageId());
        B_PLUS_TREE_INTERNAL_PAGE *parent = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE *>(page->GetData());
        parent->SetKeyAt(parent->ValueIndex(GetPageId()), array[0].first);
        buffer_pool_manager->UnpinPage(GetParentPageId(), true);
    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyLastFrom(
            const MappingType &pair, BufferPoolManager *buffer_pool_manager) {

        assert(GetSize() + 1 <= GetMaxSize());

        this->array[GetSize()] = pair;
        this->IncreaseSize(1);
    }

/*
 * Remove the last key & value pair from this page to head of "recipient"
 * page, then update relevant key & value pair in its parent page.
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveLastToFrontOf(
            BPlusTreeInternalPage *recipient, int parent_index,
            BufferPoolManager *buffer_pool_manager) {
        MappingType pair {
            this->KeyAt(this->GetSize() - 1),
            this->ValueAt(this->GetSize() - 1)
        };
        this->IncreaseSize(-1);
        recipient->CopyFirstFrom(pair, parent_index, buffer_pool_manager);
    }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::CopyFirstFrom(
            const MappingType &pair, int parent_index,
            BufferPoolManager *buffer_pool_manager) {

        assert(GetSize() + 1 < GetMaxSize());
        memmove((void*)(array + 1), array, GetSize()*sizeof(MappingType));
        this->IncreaseSize(1);
        this->array[0] = pair;

        // update child parent page id
        page_id_t childPageId = pair.second;
        Page *page = buffer_pool_manager->FetchPage(childPageId);
        assert (page != nullptr);

        BPlusTreePage *child = reinterpret_cast<BPlusTreePage *>(page->GetData());
        child->SetParentPageId(GetPageId());
        assert(child->GetParentPageId() == GetPageId());
        buffer_pool_manager->UnpinPage(child->GetPageId(), true);

        //update relevant key & value pair in its parent page.
        page = buffer_pool_manager->FetchPage(GetParentPageId());
        B_PLUS_TREE_INTERNAL_PAGE *parent = reinterpret_cast<B_PLUS_TREE_INTERNAL_PAGE *>(page->GetData());
        parent->SetKeyAt(parent_index, array[0].first);
        buffer_pool_manager->UnpinPage(GetParentPageId(), true);
    }

/*****************************************************************************
 * DEBUG
 *****************************************************************************/
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_INTERNAL_PAGE_TYPE::QueueUpChildren(
            std::queue<BPlusTreePage *> *queue,
            BufferPoolManager *buffer_pool_manager) {
        for (int i = 0; i < GetSize(); i++) {
            auto *page = buffer_pool_manager->FetchPage(array[i].second);
            if (page == nullptr)
                throw Exception(EXCEPTION_TYPE_INDEX,
                                "all page are pinned while printing");
            BPlusTreePage *node =
                    reinterpret_cast<BPlusTreePage *>(page->GetData());
            queue->push(node);
        }
    }

    INDEX_TEMPLATE_ARGUMENTS
    std::string B_PLUS_TREE_INTERNAL_PAGE_TYPE::ToString(bool verbose) const {
        if (GetSize() == 0) {
            return "";
        }
        std::ostringstream os;
        if (verbose) {
            os << "[pageId: " << GetPageId() << " parentId: " << GetParentPageId()
               << "]<" << GetSize() << "> ";
        }

        int entry = verbose ? 0 : 1;
        int end = GetSize();
        bool first = true;
        while (entry < end) {
            if (first) {
                first = false;
            } else {
                os << " ";
            }
            os << std::dec << array[entry].first.ToString();
            if (verbose) {
                os << "(" << array[entry].second << ")";
            }
            ++entry;
        }
        return os.str();
    }

// valuetype for internalNode should be page id_t
    template
    class BPlusTreeInternalPage<GenericKey<4>, page_id_t,GenericComparator<4>>;

    template
    class BPlusTreeInternalPage<GenericKey<8>, page_id_t,GenericComparator<8>>;

    template
    class BPlusTreeInternalPage<GenericKey<16>, page_id_t, GenericComparator<16>>;

    template
    class BPlusTreeInternalPage<GenericKey<32>, page_id_t,GenericComparator<32>>;

    template
    class BPlusTreeInternalPage<GenericKey<64>, page_id_t,GenericComparator<64>>;
} // namespace scudb
