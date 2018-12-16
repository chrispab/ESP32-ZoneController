#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "config.h"

#include "version.h"
#include "secret.h"

#include "MyOTA.h"
//#define SYS_FONT u8g2_font_8x13_tf
#define SYS_FONT u8g2_font_6x12_tf       // 7 px high
#define BIG_TEMP_FONT u8g2_font_fub30_tf //30px hieght

const char *ssid = MYSSID;
const char *password = MYWIFIPASSWORD;

long lastMsg = 0;
int flag = false;

/* LED */
//int led = 2;
int fanPin = 33;
int ventPin = 2;
int heaterPin = 26;

// create the display object
#include "Display.h"
#define OLED_CLOCK_PIN GPIO_NUM_22 //RHS_P_14 SCL
#define OLED_DATA_PIN GPIO_NUM_21  //RHS_P_11 SDA
Display myDisplay(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/OLED_CLOCK_PIN,
                  /* data=*/OLED_DATA_PIN);

// create object
#include "sendemail.h"

SendEmail e("smtp.gmail.com", 465, GMAILTO, GAPPPASSWORD,
            5000, true);

#include "Light.h"
#define ADC1_CH0 36
Light myLight(ADC1_CH0);

// DHT22 stuff
#define DHTPIN GPIO_NUM_25 // LHS_P_8 what digital pin we're connected to
#include <DHT.h>
DHT DHT22Sensor;


#include "Vent.h"
Vent myVent;

#include "Fan.h"
Fan myFan;

#include "Heating.h"
Heating myHeater;

void setup()
{
    Serial.begin(115200);
    Serial.println("==========running setup==========");
    /* connect to wifi */
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    /* Wait for connection */
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    /* set LED as output */
    pinMode(fanPin, OUTPUT);
    pinMode(ventPin, OUTPUT);
    pinMode(heaterPin, OUTPUT);

    setupOTA();

    // Send Email
    e.send("<cbattisson@gmail.com>", "<cbattisson@gmail.com>", "ESP32 zone controller start",
           "ESP32 zone controller  started/restarted");

    myDisplay.begin();
    myDisplay.setFont(SYS_FONT);
    myDisplay.wipe();
    myDisplay.writeLine(1, TITLE_LINE1);
    myDisplay.writeLine(2, TITLE_LINE2);
    myDisplay.writeLine(3, TITLE_LINE3);
    myDisplay.writeLine(4, TITLE_LINE4);
    myDisplay.writeLine(5, TITLE_LINE5);
    myDisplay.writeLine(6, TITLE_LINE6);
    myDisplay.refresh();
    //delay(500);

    DHT22Sensor.setup(DHTPIN, DHT22Sensor.AM2302);
}

long lastRead = 0;
bool lightState = false;
int analog_value = 0;
float temperature = 0;
float humidity = 0;
long currentMillis = 0;

//event loop
void loop()
{
   // ArduinoOTA();
    /* this function will handle incomming chunk of SW, flash and respond sender */
    ArduinoOTA.handle();
    currentMillis = millis();

    //*************************************************************************
    //REad all sensors and states
    long THnow = millis();
    if (THnow - lastRead > 5000)
    {
        lastRead = THnow;

        lightState = myLight.getLightState();
        Serial.println(lightState);
        analog_value = analogRead(ADC1_CH0);
        Serial.println(analog_value);
        //myFan.setOnMillis(analog_value);

        temperature = DHT22Sensor.getTemperature();
        Serial.println(temperature);
        humidity = DHT22Sensor.getHumidity();
        Serial.println(humidity);
    }

    //delay(5000);
    //read sensors
    //get latest I/O states
    float targetTemp;
    myLight.getLightState() ? targetTemp = TSP_LON : targetTemp = TSP_LOFF;
    

    myVent.control(temperature, targetTemp, lightState, currentMillis);
    digitalWrite(ventPin, myVent.getState());
    ArduinoOTA.handle();

    myFan.control(currentMillis);
    digitalWrite(fanPin, myFan.getState());
    //speed also
    ArduinoOTA.handle();

    myHeater.control(temperature, targetTemp, lightState, currentMillis);
    digitalWrite(heaterPin, myHeater.getState());
    ArduinoOTA.handle();

    // ctl1.board1.switch_relays(heaterState, ventState, fanState, ventSpeedState)  # switch relays according to State vars
    // stateChanged = ctl1.stateMonitor.checkForChanges(temperature, humidity, ventState,
    //                             fanState, heaterState, ventSpeedState,
    //                             current_millis, ctl1.timer1.current_time)  # write to csv/db etc if any state changes
    //if change in I/O states
    // if stateChanged :

    // sensor_data = [str(temperature), str(humidity), str(lightState)]
    // logger.warning("=============MQTT sending pre=")#, sensor_data)
    // client.publish(zone+"/TemperatureStatus", temperature)
    // client.publish(zone+"/HumidityStatus", humidity)
    // client.publish(zone+"/HeaterStatus", heaterState)

    // client.publish(zone+"/VentStatus", ventState)
    // client.publish(zone+"/FanStatus", fanState)
    // client.publish(zone+"/VentSpeedStatus", ventSpeedState)
    // client.publish(zone+"/LightStatus", lightState)
    // logger.warning("=============MQTT sending post=")#, sensor_data)

    // #print("->")
    // logger.debug("======== start state changed main list ======")
    // # check for alarm levels etc
    // if temperature > cfg.getItemValueFromConfig('tempAlertHi'):
    //     try:
    //         emailObj.send( zone + ' - Hi Temp warning' + temperature, message)
    //     except:
    //         logger.error("...ERROR SENDING EMAIL - for hi temp alert")

    // if temperature < cfg.getItemValueFromConfig('tempAlertLo'):
    //     try:
    //         emailObj.send( zone + ' - Lo Temp warning' + temperature, message)
    //     except:
    //         logger.error("...ERROR SENDING EMAIL - low temp alert")

    // location = cfg.getItemValueFromConfig('locationDisplayName')
    // logger.debug("LLLLLLLLLL - loc : %s" % (location))

    // end_time = time.time()
    // processUptime = end_time - start_time
    // processUptime = str(timedelta(seconds=int(processUptime)))
    // #cfg.setConfigItemInLocalDB('processUptime', "Process Up Time: " +processUptime)
    // cfg.setConfigItemInLocalDB('processUptime', processUptime)

    // systemUpTime = ctl1.timer1.getSystemUpTimeFromProc()
    // #cfg.setConfigItemInLocalDB('systemUpTime',  "System Up Time: " + systemUpTime)
    // cfg.setConfigItemInLocalDB('systemUpTime', systemUpTime)

    // cfg.setConfigItemInLocalDB('miscMessage', location)

    // systemMessage = ctl1.timer1.getUpTime().strip()
    // cfg.setConfigItemInLocalDB('systemMessage', systemMessage  )

    // ipAddress = get_ip_address()
    // cfg.setConfigItemInLocalDB('controllerMessage', "V: " +  VERSION + ", IP: " + "<a href=" + "https://" + ipAddress + ":10000" + ' target="_blank"' + ">"+ ipAddress + "</a>")

    // cfg.setConfigItemInLocalDB('lightState', int(lightState) )

    // #time1 = datetime.datetime.now()
    // #cfg.updateCentralConfigTable()
    // #time2 = datetime.datetime.now()
    // #duration = time2 - time1
    // #logger.warning("TTTTT - update central CONFIG table duration : %s" % (duration))

    // execAndTimeFunc(cfg.updateCentralConfigTable)

    // #uptime = cfg.getConfigItemFromLocalDB('processUptime')
    // logger.info("======== process uptime: %s ======", processUptime)
    // mem = psutil.virtual_memory()
    // #logger.warning("MMMMMM total memory       : %s MMMMMM",mem.total)

    // #logger.warning("MMMMMM memory available   : %s MMMMMM",mem.available)
    // logger.debug("MMMMMM memory pc.available: %0.2f MMMMMM",((float(mem.available)/float(mem.total)))*100)
    // #logger.warning("======== % memory available: %s ======",mem.percent)
}