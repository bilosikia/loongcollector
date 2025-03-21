/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>

#include "json/json.h"

#include "collection_pipeline/CollectionPipelineContext.h"
#include "collection_pipeline/plugin/instance/PluginInstance.h"
#include "collection_pipeline/plugin/interface/Processor.h"
#include "models/PipelineEventGroup.h"
#include "monitor/MetricManager.h"

namespace logtail {

class ProcessorInstance : public PluginInstance {
public:
    ProcessorInstance(Processor* plugin, const PluginMeta& pluginMeta) : PluginInstance(pluginMeta), mPlugin(plugin) {}

    const std::string& Name() const override { return mPlugin->Name(); };

    bool Init(const Json::Value& config, CollectionPipelineContext& context);
    void Process(std::vector<PipelineEventGroup>& logGroupList);

private:
    std::unique_ptr<Processor> mPlugin;

    CounterPtr mInEventsTotal;
    CounterPtr mOutEventsTotal;
    CounterPtr mInSizeBytes;
    CounterPtr mOutSizeBytes;
    TimeCounterPtr mTotalProcessTimeMs;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorInstanceUnittest;
    friend class ProcessorParseRegexNativeUnittest;
    friend class ProcessorParseTimestampNativeUnittest;
    friend class ProcessorParseJsonNativeUnittest;
    friend class ProcessorParseApsaraNativeUnittest;
    friend class ProcessorParseDelimiterNativeUnittest;
    friend class ProcessorFilterNativeUnittest;
    friend class ProcessorDesensitizeNativeUnittest;
    friend class ProcessorSplitLogStringNativeUnittest;
    friend class InputFileUnittest;
    friend class InputPrometheusUnittest;
    friend class PipelineUnittest;
    friend class PipelineUpdateUnittest;
#endif
};

} // namespace logtail
