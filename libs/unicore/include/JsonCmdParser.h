/* Updated to match AUTOSAR Adaptive Naming and Commenting Conventions */
#ifndef JSON_COMMAND_PARSER_H
#define JSON_COMMAND_PARSER_H

#include <string>
#include "VirtualBusCmd.h"

class VirtualBusCmd;
/**
 * @brief Abstract base class for JSON command parsers.
 */
class JsonCmdParser {
public:
    /**
     * @brief Pure virtual function to parse parameters and set them in the command.
     *
     * @param[in] command Reference to a command object that parameters will be set to.
     * @param[in] parameters JSON string representing command parameters.
     * @return True if parsing is successful, otherwise false.
     */
    virtual bool parseParameters(VirtualBusCmd& command, const std::string& parameters) = 0;

    /**
     * @brief Virtual destructor.
     */
    virtual ~JsonCmdParser() = default;
};

#endif // JSON_COMMAND_PARSER_H
