// Node-RED function node: extract the temperature value from a sensor frame.
// Input  msg.payload: "0x55, Temperature, SHT35, 24.5, 0xED"
// Output msg.payload: "24.5"
// See ../../docs/protocol.md for the frame format.
var inputData = msg.payload;
var dataArray = inputData.split(",");
if (dataArray[1].trim() == "Temperature") {
    var TempData = dataArray[3].trim();
}

msg.payload = TempData;
return msg;
