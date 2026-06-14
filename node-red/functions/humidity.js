// Node-RED function node: extract the humidity value from a sensor frame.
// Input  msg.payload: "0x55, Humidity, SHT35, 55.2, 0xED"
// Output msg.payload: "55.2"
// See ../../docs/protocol.md for the frame format.
var inputData = msg.payload;
var dataArray = inputData.split(",");
if (dataArray[1].trim() == "Humidity") {
    var HumData = dataArray[3].trim();
}

msg.payload = HumData;
return msg;
