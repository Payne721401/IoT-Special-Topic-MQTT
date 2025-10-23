var inputData=msg.payload;
var dataArray = inputData.split(",");
if (dataArray[1].trim() == "Temperature"){ 
    var TempData = dataArray[3].trim();
}

msg.payload = TempData;
return msg;