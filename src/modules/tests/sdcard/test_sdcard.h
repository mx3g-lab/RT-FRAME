#pragma once

#include "vwork.h"
#include "task_register.h"

class TestSdcard : public vwork::Periodic
{
public:
	TestSdcard() : Periodic(vwork::configs::sensor) {}

protected:
	void init() override;
	void callback() override;
};
