#pragma once

#include "config.hpp"
#include <ISmmPlugin.h>

class VipAdmin final : public ISmmPlugin, public IMetamodListener
{
  public:
	bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
	bool Unload(char* error, size_t maxlen);
	void AllPluginsLoaded();
	Config& get_config() { return m_config; }

  private:
	const char* GetAuthor();
	const char* GetName();
	const char* GetDescription();
	const char* GetURL();
	const char* GetLicense();
	const char* GetVersion();
	const char* GetDate();
	const char* GetLogTag();

	Config m_config;
};

extern VipAdmin vip_admin;