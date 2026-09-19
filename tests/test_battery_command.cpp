// Regression test for BatteryCommand.h / BatteryCommandParser.h.
//
// This pair of files never compiled and was never included anywhere in
// the project, so none of the following was ever caught:
//
//   - BatteryCommand.h #included BatteryCommandParser.h, which itself
//     #includes BatteryCommand.h back. Include guards break the infinite
//     recursion, but not the ordering problem: for a translation unit
//     that includes BatteryCommand.h first, its own #include of
//     BatteryCommandParser.h ran while BatteryStateCmd was still being
//     defined, so BatteryCommandParser.h's `dynamic_cast<BatteryStateCmd*>`
//     referenced an incomplete type. Fixed the same way
//     InverterCommand.h/.cpp already avoid the equivalent cycle with
//     InverterCommandParser.h: BatteryCommand.h only forward-declares via
//     JsonCmdParser.h, and initializeParser() is defined out-of-line in
//     the new BatteryCommand.cpp, which is free to include
//     BatteryCommandParser.h.
//   - BatteryStateCmd's constructor set `type = CommandType::Server;`,
//     but neither a `type` member nor a `CommandType::Server` enumerator
//     exists (the base class's member is the protected `type_`, and the
//     enum only has Inverter/Battery/Gateway/Json). Fixed to
//     `type_ = CommandType::Battery;`.
//   - print() called `print_base()`, but the inherited method is named
//     `printBase()`.
//   - BatteryCommandParser::parseParameters() called setNumberOfCubes(),
//     setNumOfReadyCubes(), setMinVoltage(), setMaxVoltage(), and
//     setMeanSOC() on BatteryStateCmd, none of which existed -- only the
//     matching getters did. Added the five setters.
//   - BatteryStateCmd shadowed VirtualBusCmd::logger_ the same way
//     InverterCommand did (see test_logger_propagation.cpp); fixed the
//     same way.
//
// This test exercises the real parse -> set -> get round trip through
// BatteryCommandParser so a regression here shows up as a build failure
// or a failed assertion, not silence.
#include "test_framework.h"

#include "BatteryCommand.h"
#include "BatteryCommandParser.h"

#include <string>

VB_TEST(BatteryStateCmd_HasBatteryType) {
    BatteryStateCmd cmd;
    VB_CHECK(cmd.getType() == CommandType::Battery);
}

VB_TEST(BatteryCommandParser_ParsesKnownFieldsIntoBatteryStateCmd) {
    BatteryStateCmd cmd;
    BatteryCommandParser parser;

    const std::string json = R"({
        "Cube_Num": 12,
        "Cube_OP": 10,
        "Voltage": {"MIN": 49000, "MAX": 55000},
        "SOC": {"AVG": 6000}
    })";

    VB_CHECK(parser.parseParameters(cmd, json));
    VB_CHECK(cmd.getNumberOfCubes() == 12);
    VB_CHECK(cmd.getNumberOfReadyCubes() == 10);
    VB_CHECK(cmd.getVoltageMinimum() == 49000);
    VB_CHECK(cmd.getVoltageMaximum() == 55000);
    VB_CHECK(cmd.getSOCMean() == 6000);
}

VB_TEST(BatteryCommandParser_RejectsWrongCommandType) {
    // parseParameters() dynamic_casts its VirtualBusCmd& argument to
    // BatteryStateCmd*; that cast only compiles at all once
    // BatteryStateCmd is a complete type where BatteryCommandParser.h
    // needs it to be -- exactly what the header-cycle bug above broke.
    class NotABatteryCmd : public VirtualBusCmd {
    public:
        void print() const override {}
    };

    NotABatteryCmd cmd;
    BatteryCommandParser parser;
    VB_CHECK(parser.parseParameters(cmd, "{}") == false);
}

int main() {
    return vbtest::runAll();
}
