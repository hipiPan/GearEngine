#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H
#include "ResourceManager.h"
#include "../Texture/Texture.h"
#include "../Utility/Module.h"

class TextureManager : public ResourceManager, public Module<TextureManager>
{
public:
	TextureManager();
	~TextureManager();
	std::shared_ptr<Texture> getRes(std::string name)
	{
		return std::dynamic_pointer_cast<Texture>(getResImp(name));
	}
private:

};

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
}
#endif