
#include "JsonCmdParser.h"


bool VirtualBusCmd::parse(const std::string& parameters) {
        //return parser_ ? parser_->parseParameters(*this, parameters) : false;

        if (!parser_) {
            if (logger_) {
                logger_->error("VirtualBusCmd: No parser set.");
            }
            return false;
        }
        try {
            // Assuming parser has a parse method that returns a bool
            bool result = parser_->parseParameters(*this,parameters);
            if (logger_) {
                logger_->info("VirtualBusCmd: Parsing completed with result: " + std::to_string(result));
            }
            return result;
        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("VirtualBusCmd: Parsing error: " + std::string(e.what()));
            }
            return false;
        }
    }
