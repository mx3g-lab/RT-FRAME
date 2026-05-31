#pragma once

#include <vwork.h>

class ParamLoader : public vwork::Periodic
{
public:
	ParamLoader() : vwork::Periodic(vwork::configs::param_save) {}

private:
	void init() override;
	void callback() override {}
};
