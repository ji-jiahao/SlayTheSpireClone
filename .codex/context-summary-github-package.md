# GitHub 上传与中文发行包

时间：2026-09-13
目标：上传当前已完成的音频替换、塔底及随机遭遇修复，生成“东南苦行塔”Windows x64 压缩包。
参考模式：CMakeLists.txt 的 SlayTheSpire 目标及 POST_BUILD 资源/DLL 复制；src/app/Game.cpp 的窗口标题；dist 的历史解压即玩目录；README 的构建文档。内部目标名保留以复用构建接口，通过 OUTPUT_NAME 修改发行文件名，窗口标题统一 UTF-8 转换。
计划：中文命名→Release 构建/测试→明确白名单复制可执行文件、SFML 与 VC 运行库、assets→压缩并完整性核验→本地提交相关文件→推送 origin/main。
验收：程序文件名与压缩包名为东南苦行塔；资源和 DLL 完整；Release CTest 通过；打包程序能启动；压缩条目与源目录一致；远程提交与本地一致。保留历史发行包和不相关的本地文档/临时目录。
工具补偿：指定辅助分析、规划、本地文件和 GitHub 检索工具未提供，使用本地 PowerShell、Git、CMake 完成同序记录和验证。上传已有仓库采用已配置 origin，不推测其他目的地。
