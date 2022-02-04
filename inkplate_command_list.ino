#include <map>
#include <vector>
#include "Inkplate.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Regexp.h>
#include "WifiConfig.h"

Inkplate display(INKPLATE_3BIT);

const String INT = "(%d+)";
const String FLOAT = "([%d%.]+)";
const String STRING = "\"([^\"]+)\"";
const String BOOL = "(%a+)";

// https://inkplate.readthedocs.io/en/latest/arduino.html
const std::map<String, std::vector<String>> argumentPatterns = {
    { "display", {} },
    { "clearDisplay", {} },
    { "partialUpdate", {} },
    { "clean", { INT, INT } },
    { "selectDisplayMode", { "(INKPLATE_[13]BIT)" } },
    { "fillScreen", { INT } },

    { "drawImage", { STRING, INT, INT, BOOL, BOOL } },
    { "drawPixel", { INT, INT, INT } },
    { "drawLine", { INT, INT, INT, INT, INT } },
    { "drawThickLine", { INT, INT, INT, INT, INT, FLOAT } },
    { "drawGradientLine", { INT, INT, INT, INT, INT, INT, FLOAT } },
    { "drawFastVLine", { INT, INT, INT, INT } },
    { "drawFastHLine", { INT, INT, INT, INT } },
    { "drawRect", { INT, INT, INT, INT, INT } },
    { "fillRect", { INT, INT, INT, INT, INT } },
    { "drawElipse", { INT, INT, INT, INT, INT } },
    { "fillElipse", { INT, INT, INT, INT, INT } },
    { "drawCircle", { INT, INT, INT, INT } },
    { "fillCircle", { INT, INT, INT, INT } },
    { "drawTriangle", { INT, INT, INT, INT, INT, INT, INT } },
    { "fillTriangle", { INT, INT, INT, INT, INT, INT, INT } },
    { "drawRoundRect", { INT, INT, INT, INT, INT, INT } },
    { "fillRoundRect", { INT, INT, INT, INT, INT, INT } },

    { "setTextSize", { INT } },
    { "setFont", { STRING } },
    { "setCursor", { INT, INT } },
    { "print", { STRING } },
    { "println", { STRING } },
    { "setTextWrap", { BOOL } }
};

void setup()
{
    Serial.begin(115200);
    display.begin();
    display.clearDisplay();
    display.display();
    display.setTextColor(BLACK, WHITE);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}

String getCommands() {
    HTTPClient http;
    String commandList = "";

    Serial.println("Fetching command list");
    http.begin("https://inkplate.home.dmsilva.com/commands.txt");
    int httpCode = http.GET();
    if(httpCode == HTTP_CODE_OK) {
        commandList = http.getString();
    } else {
        Serial.println("Http error: " + httpCode);
    }
    http.end();

    return commandList;
}

String getArgumentPattern(const String &command) {
    String pattern = "%(%s*";
    const auto findIter = argumentPatterns.find(command);
    if (findIter == argumentPatterns.end()) {
      Serial.print("Could not find command: ");
      Serial.println(command);
      return "%(%s*%)";
    }
    const std::vector<String> patternList = (*findIter).second;
    Serial.println("iterate over pattern");
    for (String s : patternList) {
        pattern.concat(s);
        pattern.concat("%s*,%s*");
    }
    pattern.remove(pattern.length() - 7);
    pattern.concat("%s*%)");
    return pattern;
}

int getArity(const String &command) {
    const auto findIter = argumentPatterns.find(command);
    if (findIter == argumentPatterns.end()) {
      Serial.print("Could not find command: ");
      Serial.println(command);
      return -1;
    }

    return (*findIter).second.size();
}

void handleCommand(const char * match,
                   const unsigned int length,
                   const MatchState & commandMatch) {
    static char commandBuf[100];
    static char commandArgsBuf[1000];
    commandMatch.GetCapture(commandBuf, 0);
    commandMatch.GetCapture(commandArgsBuf, 1);
    String command = String(commandBuf);

    MatchState ms;
    ms.Target(commandArgsBuf);

    String argumentPattern = getArgumentPattern(command);
    ms.Match(argumentPattern.c_str());

    if (ms.level != getArity(command)) {
        Serial.print("Wrong number of arguments for command ");
        Serial.print(command);
        Serial.print(". Expected ");
        Serial.print(getArity(command));
        Serial.print(". Got ");
        Serial.println(ms.level);
    }

    std::vector<String> argsList = {};
    static char argBuf[1000];
    for (int i = 0; i < ms.level; i++) {
        ms.GetCapture(argBuf, i);
        argsList.push_back(String(argBuf));
    }

    Serial.print(command);
    Serial.println(commandArgsBuf);

    executeCommand(command, argsList);
}

void executeCommand(const String &command, const std::vector<String> &args) {
    if (command == "display") {
        display.display();
    } else if (command == "clearDisplay") {
        display.clearDisplay();
    } else if (command == "partialUpdate") {
        display.partialUpdate();
    } else if (command == "clean") {
        display.clean(args[0].toInt(), args[1].toInt());
    } else if (command == "selectDisplayMode") {
        if (args[0] == "INKPLATE_1BIT") {
            display.selectDisplayMode(INKPLATE_1BIT);
        } else {
            display.selectDisplayMode(INKPLATE_3BIT);
        }
    } else if (command == "fillScreen") {
        display.fillScreen(args[0].toInt());
    } else if (command == "drawImage") {
        display.drawImage(args[0], args[1].toInt(), args[2].toInt(), args[3] == "true", args[4] == "true");
    } else if (command == "drawPixel") {
        display.drawPixel(args[0].toInt(), args[1].toInt(), args[2].toInt());
    } else if (command == "drawLine") {
        display.drawLine(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[4].toInt(), args[5].toInt());
    } else if (command == "drawThickLine") {
        display.drawThickLine(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt(), args[5].toFloat());
    } else if (command == "drawGradientLine") {
        display.drawGradientLine(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt(), args[5].toInt(), args[6].toFloat());
    } else if (command == "drawFastVLine") {
        display.drawFastVLine(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt());
    } else if (command == "drawFastHLine") {
        display.drawFastHLine(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt());
    } else if (command == "drawRect") {
        display.drawRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt());
    } else if (command == "fillRect") {
        display.fillRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt());
    } else if (command == "drawElipse") {
        display.drawElipse(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt());
    } else if (command == "fillElipse") {
        display.fillElipse(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt());
    } else if (command == "drawCircle") {
        display.drawCircle(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt());
    } else if (command == "fillCircle") {
        display.fillCircle(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt());
    } else if (command == "drawTriangle") {
        display.drawTriangle(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt(), args[5].toInt(), args[6].toInt());
    } else if (command == "fillTriangle") {
        display.fillTriangle(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt(), args[5].toInt(), args[6].toInt());
    } else if (command == "drawRoundRect") {
        display.drawRoundRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt(), args[5].toInt());
    } else if (command == "fillRoundRect") {
        display.fillRoundRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt(), args[5].toInt());
    } else if (command == "setTextSize") {
        display.setTextSize(args[0].toInt());
    } else if (command == "setFont") {
        Serial.println("TODO: Implement setFont");
        // display.setFont();
    } else if (command == "setCursor") {
        display.setCursor(args[0].toInt(), args[1].toInt());
    } else if (command == "print") {
        display.print(args[0]);
    } else if (command == "println") {
        display.println(args[0]);
    } else if (command == "setTextWrap") {
        display.setTextWrap(args[0] == "true");
    } 
}

String oldCommandList = "";

void loop()
{
    String commandList = getCommands();
    if (commandList != oldCommandList) {
        Serial.println("Command list changed");
        oldCommandList = commandList;
        MatchState ms;
        ms.Target((char*) commandList.c_str());
        ms.GlobalMatch("(%a+)(%b());", &handleCommand);
    } else {
        Serial.println("Command list unchanged");
    }

    delay(5000);
}
