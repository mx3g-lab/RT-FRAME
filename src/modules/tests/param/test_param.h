#pragma once

#include "vwork.h"
#include "task_register.h"

class TestParam : public vwork::Periodic
{
public:
	TestParam() : Periodic(vwork::configs::sensor) {}

protected:
	void init() override;
	void callback() override;
};
