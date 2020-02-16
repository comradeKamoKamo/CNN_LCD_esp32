/* 
NOTICE
このファイルと同じ階層に「WiFi_settings.ino」を作成し、以下のように記述してください。
const char* ssid = "***your ssid***";
const char* password = "***your password***";
パスワードはハッシュ化することが望ましいとされます。
*/

#include <Wire.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDRESS 0x27    //必要に応じて変更
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

void setup() {
    Serial.begin(9600);                 //シリアルポートは9600番
    
	lcd.begin();
    lcd.clear();
	lcd.backlight();
	lcd.print("CNN Headlines:");

    //WiFiに接続する。
    Serial.print("\nConnecting to ");
    Serial.println(ssid);
    lcd.setCursor(0,1);
    lcd.print("Connecting...   ");

    WiFi.begin(ssid, password);

    while(WiFi.status() != WL_CONNECTED) {
        //接続できない場合、再試行する。
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    
}

unsigned long last_get_time;

void loop() {
    lcd.setCursor(0,1);
    lcd.print("Updating...     ");
    String response;
    if(!get_news_from_cnn(&response)) {
        lcd.setCursor(0,1);
        lcd.print("Failed updating.");
        delay(2000);
        while(true) {delay(1);}
    };
    String news;
    if(!get_news_plaintext(response, &news)) {
        lcd.setCursor(0,1);
        lcd.print("Failed parsing. ");
        delay(2000);
        while(true) {delay(1);}
    }
    Serial.println(news);
    last_get_time = millis();
    //3分おきに更新
    unsigned int index = 0;
    bool once_end = false;
    //1周済みかつ、5分経過で更新する。
    while(millis() - last_get_time < 5*60*1000 && !once_end) {
        lcd.setCursor(0,1);
        lcd.print(get_16len_text(news, index));
        index++;
        if(index >= news.length()) {
            index = 0;
            once_end = true;
        }
        delay(500);
    }
}

String get_16len_text(String text, unsigned int index) {
    if(index + 16 > text.length()) {
        return text.substring(index) + text.substring(0, (index + 16) - text.length());
    } else {
        return text.substring(index, index + 16);
    }
}

bool get_news_plaintext(String response, String* buf) {
    //改行を消去
    response.replace("\r","");
    response.replace("\n","");

    //HTTPのヘッダをカットしてHTMLを取り出す。
    int html_index = response.indexOf("<!DOCTYPE HTML");
    if(html_index == -1) {
        //HTMLがない＝エラーコードが返っている？
        *buf = response;
        return false;
    }
    String html = response.substring(html_index);

    //<ul>～</ul>間を取り出す。
    int ul_index = html.indexOf("<ul>");
    int ul_end_index = html.indexOf("</ul>");
    if(ul_index == -1 || ul_end_index == -1) {
        //ulがない
        *buf = html;
        return false;
    }
    String ul_area = html.substring(ul_index, ul_end_index + 5);

    //</a></li>を" "に置換
    ul_area.replace("</a></li>", " ");
    //タグを削除
    String notag = ul_area;
    while(true) {
        int start = notag.indexOf('<');
        if(start == -1) break;
        int end = notag.indexOf('>', start);
        if(end == -1) break;
        notag.remove(start, end - start + 1);
    }

    //&#x27;を"'"に置換
    notag.replace("&#x27;", "'");
    *buf = notag;
    return true;
}

bool get_news_from_cnn(String* buf) {
    Serial.println("Conneting to lite.cnn.io");
    
    WiFiClient client;

    if (!client.connect("lite.cnn.io", 80)) {
        //接続失敗
        Serial.println("Connection failed.");
        return false;
    }

    String request = "GET /en HTTP/1.1\r\n";
    request += "Host: lite.cnn.io\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";

    Serial.println("HTTP Request:");
    Serial.print(request);

    client.print(request);
    unsigned long timeout = millis();   //millis()は実行開始時からの経過ミリ秒を返す。
    while(!client.available()) {        //client.available()は読み取り可能なバイト数を返す。
        if(millis() - timeout > 10000) { 
            Serial.println(">>> Client timeout.");
            client.stop();
            return false;
        }
    }

    *buf = "";
    while(client.available()) {
        *buf += client.readStringUntil('\r'); 
    }

    client.stop();
    Serial.println("Connection Closed.\n");
    return true;
}