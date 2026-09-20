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
// Two more bugs, found after the above got this compiling and running for
// the first time:
//
//   - getVoltageMean() computed `voltageMaximum + voltageMinimum / 2`,
//     which due to operator precedence is `voltageMaximum + (voltageMinimum
//     / 2)`, not `(voltageMaximum + voltageMinimum) / 2`. Parenthesized it.
//   - toJson() reports "Current" (AVG/MAX/MIN), "SOC" MIN/MAX, and
//     "Voltage" AVG, but the parser never read those JSON fields and
//     BatteryStateCmd had no setters for the members backing them
//     (currentMean/currentMinimum/currentMaximum, socMinimum/socMaximum,
//     voltageMean), so they stayed at their sentinel/zero defaults
//     forever. Added setMeanCurrent()/setMinCurrent()/setMaxCurrent(),
//     setMinSOC()/setMaxSOC(), and setMeanVoltage(), and wired the parser
//     to call them for "Current".{AVG,MIN,MAX}, "SOC".{MIN,MAX}, and
//     "Voltage".AVG. While wiring setMeanVoltage() up, a "Voltage":
//     {"AVG": 52000} round trip came back as -13536: voltageMean was
//     declared `int16_t`, which overflows for any millivolt reading above
//     32767 -- a real value for this domain (voltageMinimum/voltageMaximum
//     already had to go up near 57000). Changed voltageMean to `uint16_t`
//     to match.
//
// This test exercises the real parse -> set -> get round trip through
// BatteryCommandParser so a regression here shows up as a build failure
// or a failed assertion, not silence.
#include "test_framework.h"

#include "BatteryCommand.h"
#include "BatteryCommandParser.h"
#include "nlohmann/json.hpp"

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
        "Voltage": {"MIN": 49000, "MAX": 55000, "AVG": 52000},
        "SOC": {"AVG": 6000, "MIN": 3000, "MAX": 9000},
        "Current": {"AVG": 10, "MIN": -50, "MAX": 80}
    })";

    VB_CHECK(parser.parseParameters(cmd, json));
    VB_CHECK(cmd.getNumberOfCubes() == 12);
    VB_CHECK(cmd.getNumberOfReadyCubes() == 10);
    VB_CHECK(cmd.getVoltageMinimum() == 49000);
    VB_CHECK(cmd.getVoltageMaximum() == 55000);
    VB_CHECK(cmd.getSOCMean() == 6000);

    // These previously had no setter for the parser to call at all, so
    // they stayed stuck at their sentinel/zero defaults no matter what
    // the input JSON said.
    VB_CHECK(cmd.getSOCMinimum() == 3000);
    VB_CHECK(cmd.getSOCMaximum() == 9000);
    VB_CHECK(cmd.getCurrentMean() == 10);
    VB_CHECK(cmd.getCurrentMinimum() == -50);
    VB_CHECK(cmd.getCurrentMaximum() == 80);

    // Voltage.AVG in particular: this is where the voltageMean int16_t
    // overflow showed up (52000 read back as -13536 before the type fix).
    nlohmann::json roundTripped = cmd.toJson();
    VB_CHECK(roundTripped["Voltage"]["AVG"].get<int>() == 52000);
}

VB_TEST(BatteryStateCmd_VoltageMeanUsesCorrectOperatorPrecedence) {
    BatteryStateCmd cmd;
    cmd.setMinVoltage(49000);
    cmd.setMaxVoltage(55000);
    // (55000 + 49000) / 2 == 52000. The buggy
    // `voltageMaximum + voltageMinimum / 2` would instead compute
    // 55000 + (49000 / 2) == 79500.
    VB_CHECK(cmd.getVoltageMean() == 52000);
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
