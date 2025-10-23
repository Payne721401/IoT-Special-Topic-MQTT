var inputData = msg.payload;
var dataArray = inputData.split(",");
if (dataArray[1].trim() == "Humidity") {
    var HumData = dataArray[3].trim();
}

msg.payload = HumData;
return msg;