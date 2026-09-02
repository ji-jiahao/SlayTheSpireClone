#include "MapGenerator.hpp"
#include <cstdlib>
#include <ctime>
#include <cmath>

bool MapGenerator::PathHasShop(MapNode* node)
{
	MapNode* cur = node;
	while (cur != nullptr)
	{
		if (cur->type == RoomType::Shop)
		{
			return true;
		}
		cur = cur->parent;
	}
	return false;
}

void MapGenerator::LayoutPosition(std::vector<MapNode*>& allNodes, int totalLayer, float width, float height)
{
	std::vector<std::vector<MapNode*>> layers(totalLayer);
	for (auto n : allNodes)
	{
		layers[n->layer].push_back(n);
	}

	float layerStep = height / static_cast<float>(totalLayer - 1);
	for (int l = 0; l < totalLayer; l++)
	{
		auto& list = layers[l];
		int count = static_cast<int>(list.size());
		float gap = width / static_cast<float>(count + 1);
		for (int i = 0; i < count; i++)
		{
			list[i]->posY = l * layerStep;
			list[i]->posX = gap * static_cast<float>(i + 1);
		}
	}
}

std::vector<MapNode*> MapGenerator::GenerateMap(int totalLayer)
{
	srand(static_cast<unsigned int>(time(nullptr)));
	std::vector<MapNode*> allNodes;
	if (totalLayer <= 0)
	{
		return allNodes;
	}

	MapNode* startNode = new MapNode();
	startNode->layer = 0;
	startNode->type = RoomType::Battle;
	startNode->visited = false;
	startNode->parent = nullptr;
	startNode->reachable = true;
	allNodes.push_back(startNode);

	int branchCount = 0;
	const int maxBranch = 2;
	std::vector<std::vector<MapNode*>> layers(totalLayer);
	layers[0].push_back(startNode);

	for (int l = 0; l < totalLayer - 1; l++)
	{
		for (auto node : layers[l])
		{
			int childNum = 1;
			if (branchCount < maxBranch && rand() % 3 == 0)
			{
				childNum = 2;
				branchCount++;
			}
			for (int c = 0; c < childNum; c++)
			{
				MapNode* child = new MapNode();
				child->layer = l + 1;
				child->visited = false;
				child->parent = node;
				child->reachable = false;
				if (PathHasShop(node))
				{
					int r = rand() % 4;
					if (r == 0)
					{
						child->type = RoomType::Battle;
					}
					else if (r == 1)
					{
						child->type = RoomType::Elite;
					}
					else if (r == 2)
					{
						child->type = RoomType::Event;
					}
					else
					{
						child->type = RoomType::Rest;
					}
				}
				else
				{
					int r = rand() % 5;
					if (r == 0)
					{
						child->type = RoomType::Battle;
					}
					else if (r == 1)
					{
						child->type = RoomType::Elite;
					}
					else if (r == 2)
					{
						child->type = RoomType::Event;
					}
					else if (r == 3)
					{
						child->type = RoomType::Shop;
					}
					else
					{
						child->type = RoomType::Rest;
					}
				}
				node->children.push_back(child);
				layers[l + 1].push_back(child);
				allNodes.push_back(child);
			}
		}
	}
	LayoutPosition(allNodes, totalLayer, 800.f, 600.f);
	return allNodes;
}
