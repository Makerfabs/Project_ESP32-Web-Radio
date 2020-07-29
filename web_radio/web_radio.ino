#include "Arduino.h"
#include "Audio.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Digital I/O used  //Makerfabs Audio V2.0
#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25

//SSD1306
#define MAKEPYTHON_ESP32_SDA 4
#define MAKEPYTHON_ESP32_SCL 5
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Button
const int Pin_vol_up = 39;
const int Pin_vol_down = 36;
const int Pin_mute = 35;

const int Pin_previous = 15;
const int Pin_pause = 33;
const int Pin_next = 2;

Audio audio;

const char *ssid = "Makerfabs";
const char *password = "20160704";


struct Music_info
{
    String name;
    int length;
    int runtime;
    int volume;
    int status;
    int mute_volume;
} music_info = {"", 0, 0, 0, 0, 0};

int volume = 21;
int mute_volume = 0;

int runtime = 0;
int length = 0;

String stations[] = {
    "0n-80s.radionetz.de:8000/0n-70s.mp3",
    "mediaserv30.live-streams.nl:8000/stream",
    "www.surfmusic.de/m3u/100-5-das-hitradio,4529.m3u",
    "stream.1a-webradio.de/deutsch/mp3-128/vtuner-1a",
    "mp3.ffh.de/radioffh/hqlivestream.aac", //  128k aac
    "www.antenne.de/webradio/antenne.m3u",
    "listen.rusongs.ru/ru-mp3-128",
    "edge.audio.3qsdn.com/senderkw-mp3",
    "macslons-irish-pub-radio.com/media.asx"};
int station_index = 0;
int station_count = sizeof(stations) / sizeof(stations[0]);

void setup()
{
    //IO mode init
    pinMode(Pin_vol_up, INPUT_PULLUP);
    pinMode(Pin_vol_down, INPUT_PULLUP);
    pinMode(Pin_mute, INPUT_PULLUP);
    pinMode(Pin_previous, INPUT_PULLUP);
    pinMode(Pin_pause, INPUT_PULLUP);
    pinMode(Pin_next, INPUT_PULLUP);

    //Serial
    Serial.begin(115200);

    //LCD
    Wire.begin(MAKEPYTHON_ESP32_SDA, MAKEPYTHON_ESP32_SCL);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    display.clearDisplay();
    logoshow();

    //connect to WiFi
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" CONNECTED");
    lcd_text("Wifi CONNECT");

    //Audio(I2S)
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21); // 0...21

    open_new_radio(stations[station_index]);
}

uint run_time = 0;
uint button_time = 0;

void loop()
{
    audio.loop();
    print_song_time();

    //Display logic
    if (millis() - run_time > 1000)
    {
        run_time = millis();
        display_music();
    }

    //Button logic
    if (millis() - button_time > 300)
    {
        if (digitalRead(Pin_next) == 0)
        {
            Serial.println("Pin_next");
            if (station_index < station_count - 1)
            {
                station_index++;
            }
            else
            {
                station_index = 0;
            }
            button_time = millis();
            open_new_radio(stations[station_index]);
        }
        if (digitalRead(Pin_previous) == 0)
        {
            Serial.println("Pin_previous");
            if (station_index > 0)
            {
                station_index--;
            }
            else
            {
                station_index = station_count - 1;
            }
            button_time = millis();
            open_new_radio(stations[station_index]);
        }
        if (digitalRead(Pin_vol_up) == 0)
        {
            Serial.println("Pin_vol_up");
            if (volume < 21)
                volume++;
            audio.setVolume(volume);
            button_time = millis();
        }
        if (digitalRead(Pin_vol_down) == 0)
        {
            Serial.println("Pin_vol_down");
            if (volume > 0)
                volume--;
            audio.setVolume(volume);
            button_time = millis();
        }
        if (digitalRead(Pin_mute) == 0)
        {
            Serial.println("Pin_mute");
            if (volume != 0)
            {
                mute_volume = volume;
                volume = 0;
            }
            else
            {
                volume = mute_volume;
            }
            audio.setVolume(volume);
            button_time = millis();
        }
        if (digitalRead(Pin_pause) == 0)
        {
            Serial.println("Pin_pause");
            audio.pauseResume();
            button_time = millis();
        }
    }

    //Serial logic
    if (Serial.available())
    {
        String r = Serial.readString();
        r.trim();
        if (r.length() > 5)
        {
            audio.stopSong();
            open_new_radio(r);
        }
        else
        {
            audio.setVolume(r.toInt());
        }
    }
}

void print_song_time()
{
    runtime = audio.getAudioCurrentTime();
    length = audio.getAudioFileDuration();
}

void open_new_radio(String station)
{
    audio.connecttohost(station);
    runtime = audio.getAudioCurrentTime();
    length = audio.getAudioFileDuration();
    Serial.println("**********start a new radio************");
}

void display_music()
{
    int line_step = 24;
    int line = 0;
    char buff[20];
    ;
    sprintf(buff, "RunningTime:%d",runtime);

    display.clearDisplay();

    display.setTextSize(1);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text

    display.setCursor(0, line); // Start at top-left corner
    display.println(stations[station_index]);
    line += line_step * 2;

    display.setCursor(0, line);
    display.println(buff);
    line += line_step;

    display.display();
}

void logoshow(void)
{
    display.clearDisplay();

    display.setTextSize(2);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);             // Start at top-left corner
    display.println(F("MakePython"));
    display.setCursor(0, 20); // Start at top-left corner
    display.println(F("WEB RADIO"));
    display.setCursor(0, 40); // Start at top-left corner
    display.println(F(""));
    display.display();
    delay(2000);
}

void lcd_text(String text)
{
    display.clearDisplay();

    display.setTextSize(2);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);             // Start at top-left corner
    display.println(text);
    display.display();
    delay(500);
}

//**********************************************
// optional
void audio_info(const char *info)
{
    Serial.print("info        ");
    Serial.println(info);
}
void audio_id3data(const char *info)
{ //id3 metadata
    Serial.print("id3data     ");
    Serial.println(info);
}

void audio_eof_mp3(const char *info)
{ //end of file
    Serial.print("eof_mp3     ");
    Serial.println(info);
}
void audio_showstation(const char *info)
{
    Serial.print("station     ");
    Serial.println(info);
}
void audio_showstreaminfo(const char *info)
{
    Serial.print("streaminfo  ");
    Serial.println(info);
}
void audio_showstreamtitle(const char *info)
{
    Serial.print("streamtitle ");
    Serial.println(info);
}
void audio_bitrate(const char *info)
{
    Serial.print("bitrate     ");
    Serial.println(info);
}
void audio_commercial(const char *info)
{ //duration in sec
    Serial.print("commercial  ");
    Serial.println(info);
}
void audio_icyurl(const char *info)
{ //homepage
    Serial.print("icyurl      ");
    Serial.println(info);
}
void audio_lasthost(const char *info)
{ //stream URL played
    Serial.print("lasthost    ");
    Serial.println(info);
}
void audio_eof_speech(const char *info)
{
    Serial.print("eof_speech  ");
    Serial.println(info);
}