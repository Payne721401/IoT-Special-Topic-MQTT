var inputData = msg.payload;

var dataArray = inputData.split(",");
if(dataArray[1].trim()=="Bike01"){
    var FanData = dataArray[3].trim();
}
msg.payload = FanData;

if (FanData==0){
    msg.payload = "5";
}

if (FanData == 2) {
    msg.payload = "4";
}

if (FanData == 3) {
    msg.payload = "3";
}

if (FanData == 4) {
    msg.payload = "2";
}

if (FanData == 5) {
    msg.payload = "1";
}

if (FanData == 25) {
    msg.payload = "0";
}


return msg;