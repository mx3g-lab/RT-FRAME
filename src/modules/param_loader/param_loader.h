#pragma once

#include <vwork.h>

class ParamLoader : public vwork::Thread
{
public:
	ParamLoader() : vwork::Thread(vwork::configs::param_auto_start) {}

private:
	void run() override;
};
