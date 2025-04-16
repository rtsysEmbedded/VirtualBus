#include "InverterCommand.h"
#include "InverterCommandParser.h"
#include <memory>

void InverterCommand::initializeParser() {
    // Creating a shared pointer to InverterCommandParser and assigning it to JsonCmdParser pointer
    std::shared_ptr<JsonCmdParser> parser = std::make_shared<InverterCommandParser>();
    setParser(parser);

    if (logger_) {
        logger_->info("InverterCommand: Parser initialized.");
    }
}
