#include "AudioManager.hpp"

AudioManager& AudioManager::Instance()
{
	static AudioManager ins;
	return ins;
}

AudioManager::AudioManager()
{
	m_volume = 40.f;
}

AudioManager::~AudioManager()
{
	m_mapBgm.stop();
}

void AudioManager::LoadMapBGM(const std::string& path)
{
	m_mapBgm.openFromFile(path);
	m_mapBgm.setLoop(true);
	m_mapBgm.setVolume(m_volume);
}

void AudioManager::PlayMapBGM()
{
	m_mapBgm.play();
}

void AudioManager::StopBGM()
{
	m_mapBgm.stop();
}

void AudioManager::SetVolume(float vol)
{
	m_volume = vol;
	m_mapBgm.setVolume(m_volume);
}
