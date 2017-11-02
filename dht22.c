#include <wiringPi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_TIME 85
#define DHT11PIN 0

#define INITIAL_HOLD_LOW 18
#define INITIAL_HOLD_HIGH 40

int dht11_val[5]={0,0,0,0,0};

void print_state_transition(uint8_t old_state, uint8_t new_state, uint8_t time){
  if( old_state == LOW && new_state == HIGH ){
    fprintf(stderr, "_%d/¯", time);
  } else if( old_state == HIGH && new_state == LOW ){
    fprintf(stderr, "¯%d\\_", time);
  }
}

uint8_t checksum(uint8_t rh_int, uint8_t rh_dec, uint8_t t_int, uint8_t t_dec){
  uint32_t sum = rh_int;
  sum += rh_dec;
  sum += t_int;
  sum += t_dec;
  return sum & 0xFF;
}

void dht11_read_val(){
  uint8_t lst_state=HIGH;
  uint8_t old_state=HIGH;
  uint8_t counter=0;
  uint8_t j=0,i;

  // init dht11_val
  for(i=0;i<5;i++){
     dht11_val[i]=0;
  }

  pinMode(DHT11PIN,OUTPUT);

  digitalWrite(DHT11PIN,LOW);
  delay(INITIAL_HOLD_LOW);

  digitalWrite(DHT11PIN,HIGH);
  delayMicroseconds(INITIAL_HOLD_HIGH);

  pinMode(DHT11PIN,INPUT);

  for(i=0;i<MAX_TIME;i++){
    counter=0;
    // this is supposed to be edge detection
    while(digitalRead(DHT11PIN)==lst_state){
      counter++;
      delayMicroseconds(1);
      if(counter==255){
        break;
      }
    }
    old_state = lst_state;
    lst_state=digitalRead(DHT11PIN);
    //print_state_transition( old_state, lst_state, counter );

    if(counter==255){
       break; // we waited to long
    }

    // top 3 transistions are ignored
    if((i>=4)&&(i%2==0)){
      if( j % 8 == 0 ){
        // fprintf(stderr, "\n");
      }
      dht11_val[j/8]<<=1;
      if(counter>60){
        dht11_val[j/8]|=1;
      }
      j++;
    }
    // fprintf(stderr, " %02d", counter);
  }

  // fprintf(stderr, "\ni=%d, j=%d\n",i, j);
  // verify cheksum and print the verified data
  if( j>=39
      &&(dht11_val[4]==((dht11_val[0]+dht11_val[1]+dht11_val[2]+dht11_val[3])& 0xFF))
    ){

    time_t t;
    struct tm *tmp;
    char timeStr[200];

    t = time(NULL);
    tmp = localtime(&t);
    if( tmp != NULL && strftime(timeStr, sizeof(timeStr), "%F;%T", tmp) != 0 ){
      printf("%s;%.1f;%.1f\n", timeStr, (256 * (float) dht11_val[0] + (float) dht11_val[1] )/10.0, (256 * (float) (dht11_val[2] & 0x7F) + (float) dht11_val[3])/10.0);
    }
  }
  else{
    // we just ignore the invalid run
    // printf("Invalid Data!!\n");
  }
}

int main(void){

  // printf("Interfacing Temperature and Humidity Sensor (DHT11) With Banana Pi\n");

  if(wiringPiSetup()==-1)
    exit(1);

  while(1){
     dht11_read_val();
     delay(3000);
  }

  return 0;
}
