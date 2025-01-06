@echo off
rem Copyright 2023 iLogtail Authors
rem
rem Licensed under the Apache License, Version 2.0 (the "License");
rem you may not use this file except in compliance with the License.
rem You may obtain a copy of the License at
rem
rem      http://www.apache.org/licenses/LICENSE-2.0
rem
rem Unless required by authorised law or agreed to in writing, software
rem distributed under the License is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem See the License for the specific language governing permissions and
rem limitations under the License.


rem 设置默认的 TARGET_ARTIFACT_PATH，如果未设置，则使用当前目录下的 core\build\unittest
set TARGET_ARTIFACT_PATH=%TARGET_ARTIFACT_PATH%.\core\build\unittest


rem 定义一个搜索文件的函数
:search_files
    setlocal
    set "folder=%~1"
    for /r "%folder%" %%F in (*) do (
        if exist "%%F\" (
            rem 这是一个目录，递归调用函数
            call :search_files "%%F"
        ) else (
            rem 这是一个文件
            set "unittest=%%~nxF"
            set "unittest=!unittest:*_=!"
            if "!unittest!" == "unittest" (
                echo ============== %%~nxF ==============
                pushd "%%~dpF"
                call "%%~nxF"
                popd
                echo ====================================
            )
        )
    )
    endlocal
    goto :eof


rem 一些单元测试可能依赖于相对路径，因此在单元测试目录中执行
set "UT_BASE_PATH=%CD%\%TARGET_ARTIFACT_PATH:~2%"
set "LD_LIBRARY_PATH=%UT_BASE_PATH%;%LD_LIBRARY_PATH%"
cd /d %TARGET_ARTIFACT_PATH%
call :search_files.