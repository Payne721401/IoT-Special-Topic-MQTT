// Node-RED function node: extract the CO2 value from a sensor frame.
// The S8 sensor identifies itself with the id "0xD11704d".
// Input  msg.payload: "0x55, 0xD11704d, S8, 823, 0xED"
// Output msg.payload: "823"
// See ../../docs/protocol.md for the frame format.
var inputData = msg.payload;
var dataArray = inputData.split(",");
if (dataArray[1].trim() == "0xD11704d") {
    var co2Data = dataArray[3].trim();
}

msg.payload = co2Data;
return msg;
