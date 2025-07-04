// Copyright 2022 iLogtail Authors
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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <memory>
#include <string>

#include "common/Flags.h"
#include "file_server/EventDispatcher.h"
#include "file_server/event/Event.h"
#include "file_server/event_handler/EventHandler.h"
#include "unittest/Unittest.h"
using namespace std;

DECLARE_FLAG_STRING(ilogtail_config);

#if defined(_MSC_VER)
const char* Basepath = "\\basepath";
const char* Basepath0 = "\\basepath0";
const char* Basepath1Log = "\\basepath1\\log";
const char* Basepath01 = "\\basepath0\\1";
const char* BasepathDir = "\\basepath\\";
const char* Basepath0Dir = "\\basepath0\\";
const char* Basepath1LogDir = "\\basepath1\\log\\";
const char* Basepath01Dir = "\\basepath0\\1\\";
#else
const char* Basepath = "/basepath";
const char* Basepath0 = "/basepath0";
const char* Basepath1Log = "/basepath1/log";
const char* Basepath01 = "/basepath0/1";
const char* BasepathDir = "/basepath/";
const char* Basepath0Dir = "/basepath0/";
const char* Basepath1LogDir = "/basepath1/log/";
const char* Basepath01Dir = "/basepath0/1/";
#endif


namespace logtail {
class MockHandler : public EventHandler {
public:
    virtual void Handle(const Event& event) { ++handle_count; }
    virtual void HandleTimeOut() { ++handle_timeout_count; }
    virtual bool DumpReaderMeta(bool isRotatorReader, bool checkConfigFlag) { return true; }
    void Reset() {
        handle_count = 0;
        handle_timeout_count = 0;
    }
    int handle_count = 0;
    int handle_timeout_count = 0;
};

class EventDispatcherDirUnittest : public ::testing::Test {
protected:
    void SetUp() override {
        mHandlers.resize(10);
        for (int i = 0; i < 10; ++i) {
            std::string dir;
            if (i < 4) {
                dir = std::string(Basepath0Dir) + std::to_string(i);
            } else {
                dir = std::string(Basepath1LogDir) + std::to_string(i);
            }
            DirInfo* dirInfo = new DirInfo(dir, i, false, &mHandlers[i]);
            EventDispatcher::GetInstance()->AddOneToOneMapEntry(dirInfo, i);
        }
        // replace mTimeOutHandler
        delete EventDispatcher::GetInstance()->mTimeoutHandler;
        mTimeOutHandler = new MockHandler;
        EventDispatcher::GetInstance()->mTimeoutHandler = mTimeOutHandler;
    }

    void TearDown() override {
        mHandlers.clear();
        for (int i = 0; i < 10; ++i) {
            EventDispatcher::GetInstance()->RemoveOneToOneMapEntry(i);
        }
    }
    std::vector<MockHandler> mHandlers;
    MockHandler* mTimeOutHandler;

public:
    void TestFindAllSubDirAndHandler() {
        LOG_INFO(sLogger, ("TestFindAllSubDirAndHandler() begin", time(NULL)));
        // Case: prefix of a dir name, should find nothing
        std::string baseDir = Basepath;
        std::vector<std::pair<std::string, EventHandler*> > subdirs;
        subdirs = EventDispatcher::GetInstance()->FindAllSubDirAndHandler(baseDir);
        APSARA_TEST_TRUE_FATAL(subdirs.empty());

        // Case: match parent dir name, should find subdirs
        baseDir = Basepath0;
        subdirs.clear();
        subdirs = EventDispatcher::GetInstance()->FindAllSubDirAndHandler(baseDir);
        APSARA_TEST_EQUAL_FATAL(subdirs.size(), 4UL);

        // Case: parent dir name with ending /, should fail
        baseDir = Basepath0Dir;
        subdirs.clear();
        subdirs = EventDispatcher::GetInstance()->FindAllSubDirAndHandler(baseDir);
        APSARA_TEST_EQUAL_FATAL(subdirs.size(), 0UL);

        // Case: match lv2 parent dir name, should find subdirs
        baseDir = Basepath1Log;
        subdirs.clear();
        subdirs = EventDispatcher::GetInstance()->FindAllSubDirAndHandler(baseDir);
        APSARA_TEST_EQUAL_FATAL(subdirs.size(), 6UL);

        // Case: match whole dir name, should find itself
        baseDir = Basepath01;
        subdirs.clear();
        subdirs = EventDispatcher::GetInstance()->FindAllSubDirAndHandler(baseDir);
        APSARA_TEST_EQUAL_FATAL(subdirs.size(), 1UL);
    }

    void TestUnregisterAllDir() {
        LOG_INFO(sLogger, ("TestUnregisterAllDir() begin", time(NULL)));
        std::string baseDir = Basepath0;
        EventDispatcher::GetInstance()->UnregisterAllDir(baseDir);
        APSARA_TEST_EQUAL_FATAL(mTimeOutHandler->handle_count, 4);
    }

    void TestStopAllDir() {
        LOG_INFO(sLogger, ("TestStopAllDir() begin", time(NULL)));
        std::string baseDir = Basepath0;
        EventDispatcher::GetInstance()->StopAllDir(baseDir, "");
        for (size_t i = 0; i < 10; ++i) {
            if (i < 4) {
                APSARA_TEST_EQUAL_FATAL(mHandlers[i].handle_count, 1);
            } else {
                APSARA_TEST_EQUAL_FATAL(mHandlers[i].handle_count, 0);
            }
        }
    }
};

APSARA_UNIT_TEST_CASE(EventDispatcherDirUnittest, TestFindAllSubDirAndHandler, 0);
APSARA_UNIT_TEST_CASE(EventDispatcherDirUnittest, TestUnregisterAllDir, 0);
APSARA_UNIT_TEST_CASE(EventDispatcherDirUnittest, TestStopAllDir, 0);
} // end of namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
