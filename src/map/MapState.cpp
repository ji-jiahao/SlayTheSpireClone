#include "MapState.hpp"
#include "MapGenerator.hpp"

MapState::MapState()
{
	m_currentNode = nullptr;
}

MapState::~MapState()
{
	for(auto n : m_mapNodes)
	{
		delete n;
	}
	m_mapNodes.clear();
}

void MapState::Enter()
{
	MapGenerator gen;
	m_mapNodes = gen.GenerateMap(7);
	if (!m_mapNodes.empty())
	{
		m_currentNode = m_mapNodes[0];
		m_currentNode->visited = true;
	}
	UpdateReachable();
	AudioManager::Instance().LoadMapBGM("assets/audio/map_bgm.ogg");
	AudioManager::Instance().PlayMapBGM();
}

void MapState::Exit()
{
	AudioManager::Instance().StopBGM();
}

void MapState::UpdateReachable()
{
	for(auto n : m_mapNodes)
	{
		n->reachable = false;
	}
	if(!m_currentNode) return;
	for(auto child : m_currentNode->children)
	{
		child->reachable = true;
	}
}

void MapState::Update(float dt)
{

}

void MapState::Render()
{
	for(auto node : m_mapNodes)
	{
		for(auto child : node->children)
		{
		}
	}

	for(auto node : m_mapNodes)
	{
		switch(node->type)
		{
		case RoomType::Battle:
			break;
		case RoomType::Elite:
			break;
		case RoomType::Event:
			break;
		case RoomType::Shop:
			break;
		case RoomType::Rest:
			break;
		}
	}
}

void MapState::OnMouseClick(float mx, float my)
{
	if (!m_currentNode)
		return;
	for(auto node : m_mapNodes)
	{
		if(!node->reachable) continue;
		float dx = mx - node->posX;
		float dy = my - node->posY;
		float dist = sqrt(dx*dx + dy*dy);
		if(dist < 25.f)
		{
			m_currentNode = node;
			node->visited = true;
			UpdateReachable();
			break;
		}
	}
}

