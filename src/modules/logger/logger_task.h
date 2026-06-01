#pragma once

#include <vwork.h>

namespace px4
{
namespace logger
{
class Logger;
}
}

class LoggerTask : public vwork::Thread
{
public:
	LoggerTask() : vwork::Thread(vwork::configs::logger) {}
	~LoggerTask() override = default;

private:
	void run() override;

	px4::logger::Logger *_logger{nullptr};
};
