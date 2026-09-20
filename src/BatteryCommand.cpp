#include "BatteryCommand.h"
#include "BatteryCommandParser.h"
#include <memory>

void BatteryStateCmd::initializeParser() {
    setParser(std::make_shared<BatteryCommandParser>());
    if (logger_) {
        logger_->info("BatteryStateCmd: Parser initialized.");
    }
}
