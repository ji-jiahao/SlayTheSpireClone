# 复活音乐替换
时间：2026-09-12 16:30:45
目标：使用用户提供的 MP3 替换 heavy_is_the_crown.mp3。
已分析三个集成模式：Game.cpp 的 kHeavyCrownMusicPath 资源常量、复活过渡中的 playMusic 调用、Game::playMusic 的 SFML 文件加载；构建沿用 CMakeLists.txt 的 assets 目录复制。
验收：源文件与目标及 Debug/Release 副本哈希一致，完整音频可解码。无需修改源码或新增测试框架。指定 sequential-thinking、shrimp-task-manager、desktop-commander 工具未提供，以本地检索、本文计划与自动验证替代。
