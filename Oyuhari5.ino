/*
 * File:      Oyuhari.ino
 * Function:  お湯張り開始後、３０秒間水を捨てて、タッチセンサ（IO１４）に反応するまでお湯を溜めて、電磁弁を閉める
 *            
 * Date:      2023/11/28
 * Author:    K.Ogata
 * 
 * Hardware   MCU:  ESP32 (DOIT ESP32 DEVKIT V1 Board)
 *            G4  タッチセンサで湯船の満水を検知
 *            G14 RelayHAISUI  排水 off:close  on:排水
 *            G12 RelayYUBUNE  湯舟 off:close　on:お湯溜め
 *            
 * IP address 192.168.0.11　固定
 */
#include <WiFi.h>
#include <Arduino.h>

const char* ssid     = "EARTH";
const char* password = "64EED29965148";
String hostname = "ESP32_Oyu";
char PageMode = 'M';
String HtmlBuf;

/* 基本属性定義  */
#define SPI_SPEED   115200          // SPI通信速度

/* HTMLレスポンスヘッダーとページヘッダー */
const String strResponseHeader = "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\n"
        "Connection:close\r\n\r\n";
/* HTMLページ */
const String strHtmlHeaderA = R"rawliteral(
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">

    <meta http-equiv="Refresh" content="1">

    <link rel="icon" href="data:,">
    <style>
      html { font-family: Helvetica; display: inline-block; margin: 0px auto;text-align: center;} 
      h1 {font-size:28px;} 
      body {text-align: center;} 
      table { border-collapse: collapse; margin-left:auto; margin-right:auto; }
      th { padding: 12px; background-color: #0000cd; color: white; border: solid 2px #c0c0c0; }
      tr { border: solid 2px #c0c0c0; padding: 12px; }
      td { border: solid 2px #c0c0c0; padding: 12px; }
      .value { color:blue; font-weight: bold; padding: 1px;}
      .btn{
      color:blue; bold; padding: 1px;
            width:150px;
            font-size:20px;
      }
      .btnDis{
      color:blue; bold; padding: 1px;
            width:150px;
            font-size:20px;
            background: gray;
      }

      body {text-align: center;} 
      table { border-collapse: collapse; margin-left:auto; margin-right:auto; }
      th { padding: 12px; background-color: #0000cd; color: white; border: solid 2px #c0c0c0; }
      tr { border: solid 2px #c0c0c0; padding: 12px; }
      td { border: solid 2px #c0c0c0; padding: 12px; }
      .value { color:blue; font-weight: bold; padding: 1px;}
    </style>
  </head>
)rawliteral";

const String strHtmlHeaderM = R"rawliteral(
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">

    <meta http-equiv="Refresh" content="1">

    <link rel="icon" href="data:,">
    <style>
      html { font-family: Helvetica; display: inline-block; margin: 0px auto;text-align: center;} 
      h1 {font-size:28px;} 
      body {text-align: center;} 
      table { border-collapse: collapse; margin-left:auto; margin-right:auto; }
      th { padding: 12px; background-color: #0000cd; color: white; border: solid 2px #c0c0c0; }
      tr { border: solid 2px #c0c0c0; padding: 12px; }
      td { border: solid 2px #c0c0c0; padding: 12px; }
      .value { color:blue; font-weight: bold; padding: 1px;}
      .btn{
      color:blue; bold; padding: 1px;
            width:150px;
            font-size:20px;
      }
      .btnDis{
      color:blue; bold; padding: 1px;
            width:150px;
            font-size:20px;
            background: gray;
      }

      body {text-align: center;} 
      table { border-collapse: collapse; margin-left:auto; margin-right:auto; }
      th { padding: 12px; background-color: #0000cd; color: white; border: solid 2px #c0c0c0; }
      tr { border: solid 2px #c0c0c0; padding: 12px; }
      td { border: solid 2px #c0c0c0; padding: 12px; }
      .value { color:blue; font-weight: bold; padding: 1px;}
    </style>
  </head>
)rawliteral";

const String strHtmlBody = R"rawliteral(
  <body>

    <h1>自動お湯張りシステム</h1>
    <table>
      <tr><th>ボタン</th><th>　状　　態　</th></tr>
      <tr><td  style="text-align: center;"><a href="%ModeSW%"><button class="btn">モード</button></a></td><td  style="text-align: center;"><p>%Mode%</p></td></tr>
      <tr><td  style="text-align: center;"><a href="%HaisuiSW%"><button class="btn">排水弁</button></a></td><td  style="text-align: center;"><p>%Haisui%</p></td></tr>
      <tr><td  style="text-align: center;"><a href="%YubuneSW%"><button class="btn">湯舟弁</button></a></td><td  style="text-align: center;"><p>%Yubune%</p></td></tr>
    </table>
    <br>
    <br>
    <p style="text-align: center;"><a href="%UntenSW%"><button class="btn">%UNTEN%</button></a></p>
    <table width="300">
        <tr><th>メッセージ</th></tr>
        <tr  height="60"><td>%MESS%</td></tr>
    </table>
    
  </body>
</html>
)rawliteral";

const String ManualPage = R"rawliteral(
  <body>

    <h1>自動お湯張りシステム</h1>
    <p style="color:brown; font-weight: bold">手動モード</p>

    <table>
      <tr><th>電磁弁</th><th>状態</th></tr>
      <tr><td>電磁弁　排水</td><td  style="text-align: center;"><p>%HAISUIBEN%</p></td></tr>
      <tr><td>電磁弁　湯舟</td><td  style="text-align: center;"><p>%YUBUNEBEN%</p></td></tr>
    </table>
    <br>
    <br>
    <p style="text-align: center;"><a href="/ToAuto"><button class="btn">自動モード</button></a></p>

  </body>
</html>
)rawliteral";
 
const String AutoPage = R"rawliteral(
  <body>

    <h1>自動お湯張りシステム</h1>
    <p style="color:brown; font-weight: bold">自動モード</p>
    <p style="text-align: center;"><a href="/Start"><button class="%StartBtnClass%" %StartBtn%>お湯張り開始</button></a></p>
    <table>
      <tr><th>状　態</th><td style="text-align: center;><span class="value">%STATUS%</span></td></tr>
      <tr><th>排水弁</th><td style="text-align: center;><span class="value">%HAISUIBEN%</span></td></tr>
      <tr><th>湯舟弁</th><td style="text-align: center;><span class="value">%YUBUNEBEN%</span></td></tr>
    </table>
    <br>
    <br>
    <p style="text-align: center;"><a href="/ToManual"><button class="btn">手動モード</button></a></p>
    
  </body>
</html>
)rawliteral";

const String EndPage = R"rawliteral(
  <body>

    <h1>自動お湯張りシステム</h1>
    <p style="color:brown; font-weight: bold">お湯張り、完了しました。</p>
    <br>
    <br>
    <p style="text-align: center;"><a href="/ToManual"><button class="btn">手動モード</button></a></p>
    
  </body>
</html>
)rawliteral";

WiFiServer server(80);
#define RelayHAISUI 14
#define RelayYUBUNE 12

// Current time
unsigned long currentTime;
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//冷水排出待ち
unsigned long StartTime;
const long wait30s = 30000;

// 割り込み
hw_timer_t * timer = NULL;

// 計測結果
double Base_value = 0;
double value_ratio = 0;
String TouchResultStr;
String Sensor;
bool Mansui_Flag = false;

// 電磁弁制御
bool Yubune_Flag = false;
bool Haisui_Flag = false;

//運転モード
bool Driving_Flag = false;
String DrivingStatus = "";

// 初期化処理
void doInitialize() {
    Serial.begin(SPI_SPEED);
    delay(1000);
    Base_value = touchRead(T0);
}

void DenjiBen_Close()
{
  digitalWrite(RelayHAISUI, LOW);
  digitalWrite(RelayYUBUNE, LOW);
  Yubune_Flag = false;
  Haisui_Flag = false;

}

void DenjiBen_Init()
{
  pinMode(RelayHAISUI, OUTPUT);
  pinMode(RelayYUBUNE, OUTPUT);
  DenjiBen_Close();
}


void doSense()
{
  value_ratio = touchRead(T0)/Base_value;
  if (value_ratio < 0.9 ) 
  {
    Mansui_Flag = true;
  }
  if (value_ratio >= 1)
  {
    Mansui_Flag = false;
  }
  // Serial.println("Touch:"+ String(value_ratio) + " "+String(Mansui_Flag));
}

void DenjiBen_Oyuhari()
{
  digitalWrite(RelayHAISUI, LOW);
  Haisui_Flag = false;
  digitalWrite(RelayYUBUNE, HIGH);
  Yubune_Flag = true;
}

void DenjiBen_Haisui()
{
  digitalWrite(RelayHAISUI, HIGH);
  Haisui_Flag = true;
  digitalWrite(RelayYUBUNE, LOW);
  Yubune_Flag = false;
}

void DenjiBen_Ctl()
{
  if (PageMode == 'A') {
    doSense();
    if (Driving_Flag == true) {
      if ((millis() - StartTime) < wait30s) {
        DenjiBen_Haisui();
        DrivingStatus = "水を排水中　あと" + String( int( (wait30s - (millis() - StartTime)) /1000)  ) + "秒";
      }
      else {
        if (Mansui_Flag == false)
        {
          DenjiBen_Oyuhari();
          DrivingStatus = "お湯張り中";
        }
        else
        {
          DenjiBen_Close();
          Driving_Flag = false;
          PageMode = 'E';
        }
      }
    }
  }
  
  if (PageMode == 'M') {
    if (Yubune_Flag == true)
      digitalWrite(RelayYUBUNE, HIGH);
     else
      digitalWrite(RelayYUBUNE, LOW);
  
    if (Haisui_Flag == true)
      digitalWrite(RelayHAISUI, HIGH);
    else
      digitalWrite(RelayHAISUI, LOW);
  
  }
}

void setup()
{
  doInitialize();             // 初期化処理をして
  DenjiBen_Init();
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println(hostname);
  WiFi.mode(WIFI_STA);
  IPAddress ip(192,168,0,11);//ESP32のIPアドレスを固定する場合のアドレス
  IPAddress subnet(255,255,0,0);//ESP32のIPアドレス固定する場合のサブネット
  IPAddress gateway(192,168,0,1);//ルーターのゲートウェイを入れる
  IPAddress DNS(192,168,0,1);//DNSサーバーの設定（使用せず）
  // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.config(ip, gateway, subnet, DNS);//IP固定の設定
  WiFi.setHostname(hostname.c_str());
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
    
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &DenjiBen_Ctl, true);
  timerAlarmWrite(timer, 1000, true);
  timerAlarmEnable(timer);

}


// HTTP レスポンス処理
// 手動モード
String httpSendResponseM(String strRecvBuf) {
String buf;

  if (strRecvBuf.indexOf("GET /ToAuto") >= 0) {
    DenjiBen_Close();
    PageMode = 'A';
    DrivingStatus = "待機中";
    Mansui_Flag = false;
    Serial.println("Manual -> Auto");
    return "";
  }
    // HTTPレスポンスヘッダーを送信
    // buf = strResponseHeader;

    // ページヘッダーを送信する
    buf = buf + strHtmlHeaderM;

    // ページボディー部を編集して送信する
    buf = buf + ManualPage;


    if (strRecvBuf.indexOf("GET /HaisuiOpen") >= 0)
      Haisui_Flag = false;
    if (strRecvBuf.indexOf("GET /HaisuiClose") >= 0)
      Haisui_Flag = true;

    if (strRecvBuf.indexOf("GET /YubuneOpen") >= 0)
      Yubune_Flag = false;
    if (strRecvBuf.indexOf("GET /YubuneClose") >= 0)
      Yubune_Flag = true;    
    
    if (Haisui_Flag == true) 
        buf.replace("%HAISUIBEN%", "<a href=\"/HaisuiOpen\"><button class=\"btn\">開</button></a>");
    else
        buf.replace("%HAISUIBEN%", "<a href=\"/HaisuiClose\"><button class=\"btn\">閉</button></a>");
   
    if (Yubune_Flag ==  true)
        buf.replace("%YUBUNEBEN%", "<a href=\"/YubuneOpen\"><button class=\"btn\">開</button></a>");
    else
        buf.replace("%YUBUNEBEN%", "<a href=\"/YubuneClose\"><button class=\"btn\">閉</button></a>");

    return buf;
}

//自動モード
String httpSendResponseA(String strRecvBuf) {
String buf;

    if (strRecvBuf.indexOf("GET /ToManual") >= 0) {
      DenjiBen_Close();
      Driving_Flag = false;
      PageMode = 'M';
      Serial.println("Auto -> Manual");
      return "";
    }
      
    // HTTPレスポンスヘッダーを送信
    // buf = strResponseHeader;

    // ページヘッダーを送信する
    buf = buf + strHtmlHeaderA;

    // ページボディー部を編集して送信する
    buf = buf + AutoPage;

    if (strRecvBuf.indexOf("/Start") >= 0) {
      // Serial.println("お湯張り開始ボタンが押された。");
      if (Driving_Flag == false) {
        StartTime = millis();
        Mansui_Flag = false;
        Driving_Flag = true;
      }
    }
    
    if (Driving_Flag == true) {
        buf.replace("%StartBtn%", "disabled");
        buf.replace("%StartBtnClass%", "btnDis");
    }
    else {
        buf.replace("%StartBtn%", "");
        buf.replace("%StartBtnClass%", "btn");
    }

    if (Haisui_Flag == true) 
        buf.replace("%HAISUIBEN%", "開");
    else
        buf.replace("%HAISUIBEN%", "閉");
   
    if (Yubune_Flag ==  true)
        buf.replace("%YUBUNEBEN%", "開");
    else
        buf.replace("%YUBUNEBEN%", "閉");

   if (Mansui_Flag == true)
       buf.replace("%STATUS%", "満水");
   else
       buf.replace("%STATUS%", DrivingStatus);

    return buf;
}

//エンド
String httpSendResponseE(String strRecvBuf) {
String buf;

    if (strRecvBuf.indexOf("GET /ToManual") >= 0) {
      DenjiBen_Close();
      Driving_Flag = false;
      PageMode = 'M';
      Serial.println("End -> Manual");
      return "";
    }
      
    // HTTPレスポンスヘッダーを送信
    // buf = strResponseHeader;

    // ページヘッダーを送信する
    buf = buf + strHtmlHeaderM;

    // ページボディー部を編集して送信する
    buf = buf + EndPage;

    return buf;
}

/* HTTP リスン処理 */
void httpListen() {
String HtmlBuf;

    String strBuffer = "";
    WiFiClient client = server.available();

    if (client) {                         // クライアントから着信があれば
      currentTime = millis();
      previousTime = currentTime;
        Serial.println("New Client.");
        String currentLine = "";
        while (client.connected() && currentTime - previousTime <= timeoutTime) {      // 接続中に以下を繰り返す
          currentTime = millis();
          if (client.available()) {     // 着信データがあれば
                char c = client.read();   // 1バイト読み込んで
                strBuffer += c;           // 受信文を形成する
                if (c == '\n') {
                  // 改行文字で受信領域が空なら、レスポンス
                  // を送信してループを脱出する
                  if (currentLine.length() == 0) {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println("Connection: close");
                    client.println();

                    HtmlBuf = "";
                    while(HtmlBuf == "") {
                      if (PageMode == 'M')
                        HtmlBuf = httpSendResponseM(strBuffer);
                      else if (PageMode == 'A')
                        HtmlBuf = httpSendResponseA(strBuffer);
                      else if (PageMode == 'E')
                        HtmlBuf = httpSendResponseE(strBuffer);
                    }
                      client.println(HtmlBuf);
                      client.println();     // 最後に、HTTP終端の空行を送信

                      break;
                  } 
                  else {              // それ以外で改行なら受信領域をクリア
                     currentLine = "";
                  }
                } else if (c != '\r') {   // 改行以外なら受信領域に結合する
                      currentLine += c;
                }
            }
        }


        // コネクションを閉じる
        // currentLine = "";
        strBuffer = "";
        client.stop();
        Serial.println("Client disconnected.\n");
    }
}

void loop() {
      //doSence();
      httpListen();              // HTTPをリスンする
}
