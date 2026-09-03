# 地图模块改动说明
## 1、实现功能
1. 生成多层纵向地图，风格模仿杀戮尖塔
2. 每层房间数量随机2~4间，节点横向均匀分布，增加微小随机偏移，不僵硬
3. 布线优化：上层节点仅连接邻近下层节点，禁止远距离跨层斜线，大幅减少线条交叉
4. 限制每个下层节点最多接收2条连线，避免线条扎堆混乱
## 2、对外接口（别人如何调用我的代码）
类名：MapGenerator
函数：std::vector<MapNode*> GenerateMap(int totalLayer);
入参：totalLayer → 地图总层数
返回值：所有地图节点指针集合，每个节点包含坐标position、子节点children
## 3、文件修改清单（本次只改动下面文件）
src/map/MapGenerator.hpp
src/map/MapGenerator.cpp
## 4、注意事项
1. 无需修改main.cpp和外部其他模块，独立解耦
2. MapNode结构体仅保存坐标与子节点，本模块不做渲染，渲染由MapState完成
3. 内存：生成的MapNode由MapState析构统一释放
