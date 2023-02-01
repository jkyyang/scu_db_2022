/**
 * b_plus_tree_page.cpp
 */
#include "page/b_plus_tree_page.h"

namespace scudb {

/*
 * Helper methods to get/set page type
 * Page type enum class is defined in b_plus_tree_page.h
 */
    bool BPlusTreePage::IsLeafPage() const {
        if(this->page_type_ == IndexPageType::LEAF_PAGE) {
            return true;
        }
        return false;
    }

    bool BPlusTreePage::IsRootPage() const {
        if (this->parent_page_id_ == INVALID_PAGE_ID) {
            return true;
        }
        return false;
    }

    void BPlusTreePage::SetPageType(IndexPageType page_type) {
        this->page_type_ = page_type;
    }

/*
 * Helper methods to get/set size (number of key/value pairs stored in that
 * page)
 */
    int BPlusTreePage::GetSize() const {
        return this->size_;
    }

    void BPlusTreePage::SetSize(int size) {
        this->size_ = size;
    }

    void BPlusTreePage::IncreaseSize(int amount) {
        this->size_ += amount;
    }

/*
 * Helper methods to get/set max size (capacity) of the page
 */
    int BPlusTreePage::GetMaxSize() const {
        return this->max_size_;
    }

    void BPlusTreePage::SetMaxSize(int size) {
        this->max_size_ = size;
    }

/*
 * Helper method to get min page size
 * Generally, min page size == max page size / 2
 */
    int BPlusTreePage::GetMinSize() const {
        // if it is the root of B+ tree
        if (this->IsRootPage()){
            if(this->IsLeafPage())
                return 1;
            else
                return 2;
        }
        return this->max_size_ / 2;
    }

/*
 * Helper methods to get/set parent page id
 */
    page_id_t BPlusTreePage::GetParentPageId() const {
        return this->parent_page_id_;
    }

    void BPlusTreePage::SetParentPageId(page_id_t parent_page_id) {
        this->parent_page_id_ = parent_page_id;
    }

/*
 * Helper methods to get/set self page id
 */
    page_id_t BPlusTreePage::GetPageId() const {
        return this->page_id_;
    }

    void BPlusTreePage::SetPageId(page_id_t page_id) {
        this->page_id_ = page_id;
    }

/*
 * Helper methods to set lsn
 */
    void BPlusTreePage::SetLSN(lsn_t lsn) {
        lsn_ = lsn;
    }

/*
 * for concurrent index
 */

    bool BPlusTreePage::IsSafe(OpType op) {
        int size = this->GetSize();
        if (op == OpType::INSERT) {
            return this->GetSize() < this->GetMaxSize();
        }
        int minSize = this->GetMinSize() + 1;
        if (op == OpType::DELETE) {
            return (this->IsLeafPage()) ? size >= minSize : size > minSize;
        }
        return false;
    }

} // namespace scudb
