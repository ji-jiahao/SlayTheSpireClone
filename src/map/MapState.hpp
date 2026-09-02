#pragma once
#include "MapNode.hpp"
#include <vector>
#include "AudioManager.hpp"

class MapState
{
public:
	MapState();
	~MapState();
	void Enter();
	void Exit();
	void Update(float dt);
	void Render();
	void OnMouseClick(float mx, float my);
private:
	std::vector<MapNode*> m_mapNodes;
	MapNode* m_currentNode;
	void UpdateReachable();
};


