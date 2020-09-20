#include "Bounce2.h" // https://github.com/thomasfredericks/Bounce2/wiki

/** CONSTANTS */
const int RELAY = 2;
const int PIR = 3;
const int BUTTON = 4;

const int LONG_TURN_ON = 3;
const int BUTTON_PRESSED = 2;
const int ACTIVITY = 1;
const int NO_ACTIVITY = 0;
unsigned long currentMillis;

const long INTERVAL = 45000;
const long LONG_INTERVAL = 3600000;

int  button_pressed;
int previous_button_pressed;


Bounce debouncer_button = Bounce(); 

int state = NO_ACTIVITY;


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(PIR, INPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  digitalWrite(RELAY, LOW); 
  digitalWrite(LED_BUILTIN, LOW);   

  debouncer_button.interval(5);
  debouncer_button.attach(BUTTON);
  
  //PHOTOCELL
  Serial.begin(9600);
  Serial.println("START");
}



void checkButton(){
   debouncer_button.update();
    // Get the update value
    button_pressed = !debouncer_button.read();
    if (!previous_button_pressed && button_pressed) {
        switch(state){
        case LONG_TURN_ON :{
          state = ACTIVITY;
          digitalWrite(LED_BUILTIN, LOW); // TURN ON LIGHT

        }
        break;
        default: {
          state = BUTTON_PRESSED;
          digitalWrite(LED_BUILTIN, HIGH); // TURN ON LIGHT

        }
        break;
      }
     
    }
    previous_button_pressed = button_pressed;
}

void checkStatus(){
   //process pir
    int pirStatus = digitalRead(PIR);
    switch(state){
      case BUTTON_PRESSED:{
          currentMillis = millis();
          Serial.println("BUTTON_PRESSED");
          digitalWrite(RELAY, HIGH); 
           Serial.println("LONG_TURN_ON");
          state = LONG_TURN_ON;
      }
      break;
      case LONG_TURN_ON:{
          if(millis() > currentMillis + LONG_INTERVAL){
            Serial.println("NO_ACTIVITY");
            state = NO_ACTIVITY;
            digitalWrite(RELAY, LOW); 
            digitalWrite(LED_BUILTIN, LOW); // TURN OFF LIGHT
           }
      }
      break;
      case NO_ACTIVITY:{
        if(pirStatus == HIGH){
          currentMillis = millis();
          state = ACTIVITY;
          Serial.println("ACTIVITY");
          digitalWrite(RELAY, HIGH);  

        }
       
      }
      break;
      case ACTIVITY:{
        digitalWrite(LED_BUILTIN, HIGH); // TURN OFF LIGHT

        if(pirStatus == HIGH){
            currentMillis = millis();
            Serial.println("reset counter");           

        }
        else if(millis() > currentMillis + INTERVAL){
            Serial.println("NO_ACTIVITY");
            state = NO_ACTIVITY;
            digitalWrite(RELAY, LOW);  
         }
        digitalWrite(LED_BUILTIN, LOW); // TURN OFF LIGHT
       
      } 
      break; 
    
    }
  

}
// the loop function runs over and over again forever
void loop() {

   
   checkButton();

   checkStatus();
  delay(400);
}
