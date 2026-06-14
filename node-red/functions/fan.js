// Node-RED function node: extract the fan command and remap it for display.
// The firmware speed scale is inverted (0 = full power ... 5 = lowest, 25 = off);
// this maps it to an intuitive 0..5 display scale (0 = off ... 5 = full power).
// Input  msg.payload: "55, Bike01, M_R, 3, ED"
// Output msg.payload: "3"  (display scale)
// See ../../docs/protocol.md for the frame format and speed table.
var inputData = msg.payload;
var dataArray = inputData.split(",");
if (dataArray[1].trim() == "Bike01") {
    var FanData = dataArray[3].trim();
}
msg.payload = FanData;

if (FanData == 0)  { msg.payload = "5"; }
if (FanData == 2)  { msg.payload = "4"; }
if (FanData == 3)  { msg.payload = "3"; }
if (FanData == 4)  { msg.payload = "2"; }
if (FanData == 5)  { msg.payload = "1"; }
if (FanData == 25) { msg.payload = "0"; }

return msg;
