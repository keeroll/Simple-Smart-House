/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT SwSerial


#include <SoftwareSerial.h>
#include <stDHT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

SoftwareSerial SwSerial(2, 1); // RX, TX (10, 11)
    
#include <BlynkSimpleStream.h>


// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "5ce3c0ac4b0647199b1c8527e24e5cb6";

DHT sensorDHT(DHT11);

const int pinDHT = 8;
const int ledPinOutside = 4;
const int pinHCSR = 6;
const int ledPinInside = 3;
const int lightPin = A0;
const int servoPin = 7;
const int buzzerPin = 5;

#define SS_PIN 10
#define RST_PIN 9
#define SP_PIN 8

MFRC522 rfid(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

Servo servo;

void setup()
{
  // Debug console
  SwSerial.begin(9600);

  // Blynk will work through Serial
  // Do not read or write this serial manually in your sketch
  Serial.begin(9600);
  Blynk.begin(Serial, auth);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(buzzerPin, OUTPUT);
  pinMode(lightPin, INPUT);
  pinMode(pinDHT, INPUT);
  pinMode(ledPinOutside, OUTPUT);
  pinMode(ledPinInside, OUTPUT);
  pinMode(pinHCSR, INPUT);
  
  digitalWrite(pinDHT, HIGH);

  servo.attach(servoPin);
  servo.write(90);

  Blynk.virtualWrite(V3, 0);

  delay(2000);
}

void loop()
{
  Blynk.run();
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
  getDhtData();

  motionDetection();

  houseAutoLighting();

  protection();
}

void getDhtData()
{
  float humidity = sensorDHT.readHumidity(pinDHT);
  float temperature = sensorDHT.readTemperature(pinDHT);

  if (isnan(humidity) || isnan(temperature))
  {
    SwSerial.println("Failed to read from DHT sensor!");
    return;
  }

  Blynk.virtualWrite(V5, humidity);
  Blynk.virtualWrite(V6, temperature);
  delay(2000);
}

void motionDetection()
{
  int hcsrValue = digitalRead(pinHCSR);

  if(hcsrValue == HIGH)
  {
    digitalWrite(ledPinOutside, HIGH);
    Blynk.virtualWrite(V0, String("Outside activity!"));
    delay(500);
  }
  else if(hcsrValue == LOW)
  {
    digitalWrite(ledPinOutside, LOW);
    Blynk.virtualWrite(V0, String(" "));
    delay(500);
  }
}
void houseAutoLighting()
{
  int lightValue = analogRead(lightPin);
  Blynk.virtualWrite(V1, lightValue);

  /*
  if(lightValue <= 650)
    digitalWrite(ledPinInside, HIGH);
  else
    digitalWrite(ledPinInside, LOW);
   */

  //lightValue = constrain(lightValue, 600, 800);
  //int ledLevel = map(lightValue, 600, 800, 255, 0);

  lightValue = constrain(lightValue, 800, 900);
  int ledLevel = map(lightValue, 800, 900, 255, 0);

  analogWrite(ledPinInside, ledLevel);
  Blynk.virtualWrite(V2, ledLevel);
  delay(500);
}

void protection()
{
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  return;

  // Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) 
  {
    //Serial.println(F("Your tag is not of type MIFARE Classic."));
    Blynk.virtualWrite(V0, "Your tag is not of type MIFARE Classic.");
    return;
  }

  String strID = "";
  for (byte i = 0; i < 4; i++) 
  {
    strID +=
    (rfid.uid.uidByte[i] < 0x10 ? "0" : "") +
    String(rfid.uid.uidByte[i], HEX) +
    (i!=3 ? ":" : "");
  }
  strID.toUpperCase();

  //Serial.print("Tap card key: ");
  //Serial.println(strID);
  Blynk.virtualWrite(V0, String("ID: ") + strID);
  

  if (strID.indexOf("50:D5:0E:7C") >= 0)
  {
    Blynk.virtualWrite(V3, 0);
    Blynk.virtualWrite(V1,V3);
    
    int tmpOk = 0;
    while(tmpOk <= 3)
    {
      ledOnOff(500);
      tmpOk++;
    }

    moveServo();

    tmpOk = 0;
    
    while(tmpOk <= 3)
    {
      ledOnOff(500);
      tmpOk++;
    }
    
    
    return;
  }
  else
  {
    Blynk.virtualWrite(V3, 1);
    Blynk.virtualWrite(V1, V3);
    Blynk.virtualWrite(V0, "Alarm!");

    int tmpAlarm = 0;

    while(tmpAlarm <= 20)
    {
      ledOnOff(100);
      tone(buzzerPin, 1);
      delay(100);
      noTone(buzzerPin);
      tmpAlarm++;
    }
    
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void moveServo()
{
  servo.write(0);
  delay(10000);
  Blynk.virtualWrite(V1, "Door's opened!");
  servo.write(90);
  delay(1000);
  Blynk.virtualWrite(V1,"Door's closed!");
}

void ledOnOff(int n)
{
  digitalWrite(ledPinOutside, HIGH);
  digitalWrite(ledPinInside, HIGH);
  delay(n);
      
  digitalWrite(ledPinOutside, LOW);
  digitalWrite(ledPinInside, LOW);
  delay(n);
}
  
BLYNK_WRITE(V3)
{
  int pinValue = param.asInt();
}
    


