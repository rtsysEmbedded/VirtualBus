// Regression test for VirtualBusCmd::serializePayload()/deserializePayload()
// and the InverterCommand/BatteryStateCmd overrides RemoteBridge relies on
// to round-trip a command's fields across a transport.
#include "test_framework.h"

#include "InverterCommand.h"
#include "BatteryCommand.h"

namespace {

// A command type that never overrides serializePayload()/deserializePayload(),
// to pin down VirtualBusCmd's default (no-op, backward-compatible) behavior.
class NoOverridePayloadCmd : public VirtualBusCmd {
public:
    void print() const override {}
};

} // namespace

VB_TEST(VirtualBusCmd_DefaultSerializePayloadIsAnEmptyObject) {
    NoOverridePayloadCmd cmd;
    nlohmann::json payload = cmd.serializePayload();
    VB_CHECK(payload.is_object());
    VB_CHECK(payload.empty());
}

VB_TEST(VirtualBusCmd_DefaultDeserializePayloadAcceptsAnythingAndSucceeds) {
    NoOverridePayloadCmd cmd;
    VB_CHECK(cmd.deserializePayload(nlohmann::json::object()) == true);
    VB_CHECK(cmd.deserializePayload(nlohmann::json{{"unexpected", 1}}) == true);
}

VB_TEST(InverterCommand_SerializePayloadRoundTripsVoltageCurrentMode) {
    InverterCommand src;
    src.setVoltage(415.5);
    src.setCurrent(-7.25);
    src.setMode(InverterCommand::Mode::Discharging);

    nlohmann::json payload = src.serializePayload();

    InverterCommand dst;
    VB_CHECK(dst.getVoltage() == 0.0); // sanity: dst starts at defaults
    VB_CHECK(dst.deserializePayload(payload));
    VB_CHECK(dst.getVoltage() == 415.5);
    VB_CHECK(dst.getCurrent() == -7.25);
    VB_CHECK(dst.getMode() == InverterCommand::Mode::Discharging);
}

VB_TEST(InverterCommand_DeserializePayloadRejectsUnknownModeString) {
    InverterCommand src;
    src.setVoltage(100.0);
    nlohmann::json payload = src.serializePayload();
    payload["mode"] = "Sideways";

    InverterCommand dst;
    VB_CHECK(dst.deserializePayload(payload) == false);
}

VB_TEST(InverterCommand_DeserializePayloadIgnoresMissingFields) {
    // A payload that only carries "voltage" (e.g. produced by a future
    // peer that adds new payload fields VirtualBusCmd doesn't understand,
    // or simply a hand-built partial payload) must not touch current/mode.
    InverterCommand dst;
    dst.setCurrent(42.0);
    dst.setMode(InverterCommand::Mode::Discharging);

    nlohmann::json partial;
    partial["voltage"] = 12.0;
    VB_CHECK(dst.deserializePayload(partial));
    VB_CHECK(dst.getVoltage() == 12.0);
    VB_CHECK(dst.getCurrent() == 42.0); // unchanged
    VB_CHECK(dst.getMode() == InverterCommand::Mode::Discharging); // unchanged
}

VB_TEST(BatteryStateCmd_SerializePayloadRoundTripsAllFields) {
    BatteryStateCmd src;
    src.setNumberOfCubes(9);
    src.setNumOfReadyCubes(7);
    src.setMinVoltage(49500);
    src.setMaxVoltage(54500);
    src.setMeanVoltage(52000);
    src.setMinSOC(2000);
    src.setMaxSOC(8000);
    src.setMeanSOC(5000);
    src.setMeanCurrent(15);
    src.setMinCurrent(-30);
    src.setMaxCurrent(45);

    nlohmann::json payload = src.serializePayload();
    // serializePayload() is toJson(): the same Voltage.AVG field that
    // once overflowed as int16_t before that bug was fixed.
    VB_CHECK(payload["Voltage"]["AVG"].get<int>() == 52000);

    BatteryStateCmd dst;
    VB_CHECK(dst.deserializePayload(payload));
    VB_CHECK(dst.getNumberOfCubes() == 9);
    VB_CHECK(dst.getNumberOfReadyCubes() == 7);
    VB_CHECK(dst.getVoltageMinimum() == 49500);
    VB_CHECK(dst.getVoltageMaximum() == 54500);
    VB_CHECK(dst.getSOCMinimum() == 2000);
    VB_CHECK(dst.getSOCMaximum() == 8000);
    VB_CHECK(dst.getSOCMean() == 5000);
    VB_CHECK(dst.getCurrentMean() == 15);
    VB_CHECK(dst.getCurrentMinimum() == -30);
    VB_CHECK(dst.getCurrentMaximum() == 45);
}

VB_TEST(BatteryStateCmd_DeserializePayloadRejectsNonObject) {
    BatteryStateCmd dst;
    VB_CHECK(dst.deserializePayload(nlohmann::json::array()) == false);
    VB_CHECK(dst.deserializePayload(nlohmann::json(42)) == false);
}

VB_TEST(BatteryStateCmd_DeserializePayloadIgnoresMissingNestedObjects) {
    BatteryStateCmd dst;
    dst.setMinVoltage(49000);
    // A payload missing "Voltage" (e.g. only "Cube_Num" set) must not
    // crash or clobber fields it doesn't mention.
    nlohmann::json partial;
    partial["Cube_Num"] = 3;
    VB_CHECK(dst.deserializePayload(partial));
    VB_CHECK(dst.getNumberOfCubes() == 3);
    VB_CHECK(dst.getVoltageMinimum() == 49000); // unchanged
}

int main() {
    return vbtest::runAll();
}
