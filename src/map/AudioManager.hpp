#pragma once
#include <SFML/Audio.hpp>

class AudioManager
{
public:
	static AudioManager& Instance();
	void LoadMapBGM(const std::string& path);
	void PlayMapBGM();
	void StopBGM();
	void SetVolume(float vol);

private:
	AudioManager();
	~AudioManager();
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	sf::Music m_mapBgm;
	float m_volume;
};
