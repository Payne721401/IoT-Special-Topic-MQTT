var inputData = msg.payload; 

var dataArray = inputData.split(",");
if (dataArray[1].trim() == "0xD11704d"){ 
    var co2Data = dataArray[3].trim();
}
msg.payload = co2Data;

return msg;