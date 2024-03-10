#include <core/Texture.h>
#include <binding/Context.h>
#include <core/Element.h>
#include <unordered_map>
extern "C" {
	#include <textureman.h>
}
namespace Rml::Texture {

using TextureMap = std::unordered_map<std::string, TextureData*>;
static TextureMap textures;

void Shutdown() {
	for (auto iterator = textures.begin(); iterator != textures.end(); ++iterator) {
		if (iterator->second != nullptr){
			delete iterator->second;
		}
		iterator->second = nullptr;
	}
	textures.clear();
}

static TextureData InvalidTexture;

TextureData* Fetch(Element* e, const std::string& path) {
	auto iterator = textures.find(path);
	if (iterator != textures.end()) {
		return iterator->second;
	}
	Rml::GetScript()->OnLoadTexture(e->GetOwnerDocument(), e, path);
	return &InvalidTexture;
}

TextureData* Fetch(Element* e, const std::string& path, Size size) {
	auto iterator = textures.find(path);
	Rml::GetScript()->OnLoadTexture(e->GetOwnerDocument(), e, path, size);
	if (iterator != textures.end()) {
		return iterator->second;
	}
	else{
		return &InvalidTexture;
	}
}

void Set(const std::string& path, TextureData*&& data) {
	auto iterator = textures.find(path);
	if (iterator != textures.end()) {
		iterator->second = data;
	}
	else{
		textures.emplace(path, std::move(data));
	}
}

TextureType GetType(TextureId id) {
	return (TextureType)texture_type(id);
}

}
