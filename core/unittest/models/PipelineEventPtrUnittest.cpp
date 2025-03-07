// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdlib>

#include "models/PipelineEventGroup.h"
#include "models/PipelineEventPtr.h"
#include "unittest/Unittest.h"

namespace logtail {

class PipelineEventPtrUnittest : public ::testing::Test {
public:
    void TestIs();
    void TestGet();
    void TestCast();
    void TestRelease();
    void TestCopy();

protected:
    void SetUp() override {
        mSourceBuffer.reset(new SourceBuffer);
        mEventGroup.reset(new PipelineEventGroup(mSourceBuffer));
    }

private:
    std::shared_ptr<SourceBuffer> mSourceBuffer;
    std::unique_ptr<PipelineEventGroup> mEventGroup;
};

void PipelineEventPtrUnittest::TestIs() {
    PipelineEventPtr logEventPtr(mEventGroup->CreateLogEvent(), false, nullptr);
    PipelineEventPtr metricEventPtr(mEventGroup->CreateMetricEvent(), false, nullptr);
    PipelineEventPtr spanEventPtr(mEventGroup->CreateSpanEvent(), false, nullptr);
    PipelineEventPtr rawEventPtr(mEventGroup->CreateRawEvent(), false, nullptr);
    APSARA_TEST_TRUE_FATAL(logEventPtr.Is<LogEvent>());
    APSARA_TEST_FALSE_FATAL(logEventPtr.Is<MetricEvent>());
    APSARA_TEST_FALSE_FATAL(logEventPtr.Is<SpanEvent>());
    APSARA_TEST_FALSE_FATAL(logEventPtr.Is<RawEvent>());

    APSARA_TEST_FALSE_FATAL(metricEventPtr.Is<LogEvent>());
    APSARA_TEST_TRUE_FATAL(metricEventPtr.Is<MetricEvent>());
    APSARA_TEST_FALSE_FATAL(metricEventPtr.Is<SpanEvent>());
    APSARA_TEST_FALSE_FATAL(metricEventPtr.Is<RawEvent>());

    APSARA_TEST_FALSE_FATAL(spanEventPtr.Is<LogEvent>());
    APSARA_TEST_FALSE_FATAL(spanEventPtr.Is<MetricEvent>());
    APSARA_TEST_TRUE_FATAL(spanEventPtr.Is<SpanEvent>());
    APSARA_TEST_FALSE_FATAL(spanEventPtr.Is<RawEvent>());

    APSARA_TEST_FALSE_FATAL(rawEventPtr.Is<LogEvent>());
    APSARA_TEST_FALSE_FATAL(rawEventPtr.Is<MetricEvent>());
    APSARA_TEST_FALSE_FATAL(rawEventPtr.Is<SpanEvent>());
    APSARA_TEST_TRUE_FATAL(rawEventPtr.Is<RawEvent>());
}

void PipelineEventPtrUnittest::TestGet() {
    auto logUPtr = mEventGroup->CreateLogEvent();
    auto addr = logUPtr.get();
    PipelineEventPtr logEventPtr(std::move(logUPtr), false, nullptr);
    APSARA_TEST_EQUAL_FATAL(addr, logEventPtr.Get<LogEvent>());
}

void PipelineEventPtrUnittest::TestCast() {
    auto logUPtr = mEventGroup->CreateLogEvent();
    auto addr = logUPtr.get();
    PipelineEventPtr logEventPtr(std::move(logUPtr), false, nullptr);
    APSARA_TEST_EQUAL_FATAL(addr, &logEventPtr.Cast<LogEvent>());
}

void PipelineEventPtrUnittest::TestRelease() {
    auto logUPtr = mEventGroup->CreateLogEvent();
    auto* addr = logUPtr.get();
    PipelineEventPtr logEventPtr(std::move(logUPtr), false, nullptr);
    APSARA_TEST_EQUAL_FATAL(addr, logEventPtr.Release());
    delete addr;
}

void PipelineEventPtrUnittest::TestCopy() {
    mEventGroup->AddLogEvent();
    mEventGroup->AddMetricEvent();
    mEventGroup->AddSpanEvent();
    mEventGroup->AddRawEvent();
    {
        auto& event = mEventGroup->MutableEvents()[0];
        event->SetTimestamp(12345678901);
        auto res = event.Copy();
        APSARA_TEST_NOT_EQUAL(event.Get<LogEvent>(), res.Get<LogEvent>());
        APSARA_TEST_FALSE(res.IsFromEventPool());
        APSARA_TEST_EQUAL(nullptr, res.GetEventPool());
    }
    {
        auto& event = mEventGroup->MutableEvents()[1];
        event->SetTimestamp(12345678901);
        auto res = event.Copy();
        APSARA_TEST_NOT_EQUAL(event.Get<MetricEvent>(), res.Get<MetricEvent>());
        APSARA_TEST_FALSE(res.IsFromEventPool());
        APSARA_TEST_EQUAL(nullptr, res.GetEventPool());
    }
    {
        auto& event = mEventGroup->MutableEvents()[2];
        event->SetTimestamp(12345678901);
        auto res = event.Copy();
        APSARA_TEST_NOT_EQUAL(event.Get<SpanEvent>(), res.Get<SpanEvent>());
        APSARA_TEST_FALSE(res.IsFromEventPool());
        APSARA_TEST_EQUAL(nullptr, res.GetEventPool());
    }
    {
        auto& event = mEventGroup->MutableEvents()[3];
        event->SetTimestamp(12345678901);
        auto res = event.Copy();
        APSARA_TEST_NOT_EQUAL(event.Get<RawEvent>(), res.Get<RawEvent>());
        APSARA_TEST_FALSE(res.IsFromEventPool());
        APSARA_TEST_EQUAL(nullptr, res.GetEventPool());
    }
}

UNIT_TEST_CASE(PipelineEventPtrUnittest, TestIs)
UNIT_TEST_CASE(PipelineEventPtrUnittest, TestGet)
UNIT_TEST_CASE(PipelineEventPtrUnittest, TestCast)
UNIT_TEST_CASE(PipelineEventPtrUnittest, TestRelease)
UNIT_TEST_CASE(PipelineEventPtrUnittest, TestCopy)

} // namespace logtail

UNIT_TEST_MAIN
