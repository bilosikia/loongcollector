// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "collection_pipeline/queue/SenderQueue.h"

#include "logger/Logger.h"

using namespace std;

namespace logtail {

SenderQueue::SenderQueue(
    size_t cap, size_t low, size_t high, QueueKey key, const string& flusherId, const CollectionPipelineContext& ctx)
    : QueueInterface(key, cap, ctx), BoundedSenderQueueInterface(cap, low, high, key, flusherId, ctx) {
    mQueue.resize(cap);
    mFetchTimesCnt = mMetricsRecordRef.CreateCounter(METRIC_COMPONENT_QUEUE_FETCH_TIMES_TOTAL);
    mValidFetchTimesCnt = mMetricsRecordRef.CreateCounter(METRIC_COMPONENT_QUEUE_VALID_FETCH_TIMES_TOTAL);
    mFetchedItemsCnt = mMetricsRecordRef.CreateCounter(METRIC_COMPONENT_QUEUE_FETCHED_ITEMS_TOTAL);
    WriteMetrics::GetInstance()->CommitMetricsRecordRef(mMetricsRecordRef);
}

bool SenderQueue::Push(unique_ptr<SenderQueueItem>&& item) {
    item->mFirstEnqueTime = chrono::system_clock::now();
    auto size = item->mData.size();

    ADD_COUNTER(mInItemsTotal, 1);
    ADD_COUNTER(mInItemDataSizeBytes, size);

    if (Full()) {
        mExtraBuffer.push_back(std::move(item));

        SET_GAUGE(mExtraBufferSize, mExtraBuffer.size());
        ADD_GAUGE(mExtraBufferDataSizeBytes, size);
        return true;
    }

    size_t index = mRead;
    for (; index < mWrite; ++index) {
        if (mQueue[index % mCapacity] == nullptr) {
            break;
        }
    }
    mQueue[index % mCapacity] = std::move(item);
    if (index == mWrite) {
        ++mWrite;
    }
    ++mSize;
    ChangeStateIfNeededAfterPush();

    SET_GAUGE(mQueueSizeTotal, Size());
    ADD_GAUGE(mQueueDataSizeByte, size);
    SET_GAUGE(mValidToPushFlag, IsValidToPush());
    return true;
}

bool SenderQueue::Remove(SenderQueueItem* item) {
    if (item == nullptr) {
        return false;
    }

    size_t size = 0;
    chrono::system_clock::time_point enQueuTime;
    auto index = mRead;
    for (; index < mWrite; ++index) {
        if (mQueue[index % mCapacity].get() == item) {
            size = item->mData.size();
            enQueuTime = item->mFirstEnqueTime;
            mQueue[index % mCapacity].reset();
            break;
        }
    }
    if (index == mWrite) {
        return false;
    }
    while (mRead < mWrite && mQueue[mRead % mCapacity] == nullptr) {
        ++mRead;
    }
    --mSize;

    ADD_COUNTER(mOutItemsTotal, 1);
    ADD_COUNTER(mTotalDelayMs, chrono::system_clock::now() - enQueuTime);
    SUB_GAUGE(mQueueDataSizeByte, size);

    if (!mExtraBuffer.empty()) {
        auto newSize = mExtraBuffer.front()->mData.size();
        PushFromExtraBuffer(std::move(mExtraBuffer.front()));
        mExtraBuffer.pop_front();

        SET_GAUGE(mExtraBufferSize, mExtraBuffer.size());
        SUB_GAUGE(mExtraBufferDataSizeBytes, newSize);
        return true;
    }
    if (ChangeStateIfNeededAfterPop()) {
        GiveFeedback();
    }

    SET_GAUGE(mQueueSizeTotal, Size());
    SET_GAUGE(mValidToPushFlag, IsValidToPush());
    return true;
}

void SenderQueue::GetAvailableItems(vector<SenderQueueItem*>& items, int32_t limit) {
    ADD_COUNTER(mFetchTimesCnt, 1);
    if (Empty()) {
        return;
    }
    bool hasAvailableItem = false;
    if (limit < 0) {
        for (auto index = mRead; index < mWrite; ++index) {
            SenderQueueItem* item = mQueue[index % mCapacity].get();
            if (item == nullptr) {
                continue;
            }
            ADD_COUNTER(mFetchedItemsCnt, 1);
            if (item->mStatus.load() == SendingStatus::IDLE) {
                item->mStatus = SendingStatus::SENDING;
                items.emplace_back(item);
                hasAvailableItem = true;
            }
        }
    } else {
        for (auto index = mRead; index < mWrite; ++index) {
            SenderQueueItem* item = mQueue[index % mCapacity].get();
            if (item == nullptr) {
                continue;
            }
            if (item->mStatus.load() != SendingStatus::IDLE) {
                continue;
            }
            hasAvailableItem = true;
            if (limit == 0) {
                break;
            }
            if (mRateLimiter && !mRateLimiter->IsValidToPop()) {
                ADD_COUNTER(mFetchRejectedByRateLimiterTimesCnt, 1);
                break;
            }
            bool rejectedByConcurrencyLimiter = false;
            for (auto& limiter : mConcurrencyLimiters) {
                if (!limiter.first->IsValidToPop()) {
                    ADD_COUNTER(limiter.second, 1);
                    rejectedByConcurrencyLimiter = true;
                    break;
                }
            }
            if (rejectedByConcurrencyLimiter) {
                break;
            }

            ADD_COUNTER(mFetchedItemsCnt, 1);
            item->mStatus = SendingStatus::SENDING;
            items.emplace_back(item);
            for (auto& limiter : mConcurrencyLimiters) {
                if (limiter.first != nullptr) {
                    limiter.first->PostPop();
                }
            }
            if (mRateLimiter) {
                mRateLimiter->PostPop(item->mRawSize);
            }
            --limit;
        }
    }
    if (hasAvailableItem) {
        ADD_COUNTER(mValidFetchTimesCnt, 1);
    }
}

void SenderQueue::SetPipelineForItems(const std::shared_ptr<CollectionPipeline>& p) const {
    if (Empty()) {
        return;
    }
    for (auto index = mRead; index < mWrite; ++index) {
        auto realIndex = index % mCapacity;
        if (!mQueue[realIndex]) {
            continue;
        }
        if (!mQueue[realIndex]->mPipeline) {
            mQueue[realIndex]->mPipeline = p;
        }
    }
    for (auto& item : mExtraBuffer) {
        if (!item->mPipeline) {
            item->mPipeline = p;
        }
    }
}

void SenderQueue::PushFromExtraBuffer(std::unique_ptr<SenderQueueItem>&& item) {
    auto size = item->mData.size();

    size_t index = mRead;
    for (; index < mWrite; ++index) {
        if (mQueue[index % mCapacity] == nullptr) {
            break;
        }
    }
    mQueue[index % mCapacity] = std::move(item);
    if (index == mWrite) {
        ++mWrite;
    }
    ++mSize;

    SET_GAUGE(mQueueSizeTotal, Size());
    ADD_GAUGE(mQueueDataSizeByte, size);
}

} // namespace logtail
