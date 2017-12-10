#include <wiringPi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_TIME 85
#define DHT11PIN 0

#define INITIAL_HOLD_LOW 18
#define INITIAL_HOLD_HIGH 40


/**
 * Helper function that prints state transitions in an ASCII
 * inspired format.
 */
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

typedef struct {

  time_t time;
  float humidity;
  float temperature;

} sensor_reading;

bool dht22_read_val( sensor_reading *sr ){

  int dht22_val[5]={0,0,0,0,0};
  uint8_t lst_state=HIGH;
  uint8_t old_state=HIGH;
  uint8_t counter=0;
  uint8_t j=0,i;

  // init dht22_val
  for(i=0;i<5;i++){
     dht22_val[i]=0;
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
      dht22_val[j/8]<<=1;
      if(counter>60){
        dht22_val[j/8]|=1;
      }
      j++;
    }
    // fprintf(stderr, " %02d", counter);
  }

  // fprintf(stderr, "\ni=%d, j=%d\n",i, j);
  // verify cheksum and print the verified data
  if( j>=39
      &&(dht22_val[4]==((dht22_val[0]+dht22_val[1]+dht22_val[2]+dht22_val[3])& 0xFF))
    ){
    // valid run, checksum checks out

    sr->time = time(NULL);
    sr->humidity = (256 * (float) dht22_val[0] + (float) dht22_val[1] )/10.0;
    sr->temperature = (256 * (float) (dht22_val[2] & 0x7F) + (float) dht22_val[3])/10.0;

    return true;
  }
  else{
    // invalid run
    return false;
  }
}

/**
 * Aquiring a sensor reading is very time sensistive and fails
 * often, depending on the load currently on the machine.
 *
 * This method allow to retry aquiring a sensor reading N times.
 *
 * On error -1 is returned and sr contains no sensible data.
 * On success 0 is returned and sr contains the timestamp of the
 * reading, humidity and temperature as returned by the sensor.
 */
int dht22_read_val_with_retries( sensor_reading *sr, uint8_t retries ){

  int result = -1;

  while( 0 < retries && 0 > (result = dht22_read_val( sr ) ) ){
    retries--;
  }

  return result;

}

void print_sensor_reading( FILE *stream, sensor_reading *sr ){

    struct tm *tmp;
    char time_str[200];

    tmp = localtime(& sr->time);
    if( NULL != tmp && strftime(time_str, sizeof(time_str), "%F;%T", tmp) != 0 ){
      fprintf( stream, "%s;%.1f;%.1f\n", time_str, sr->humidity, sr->temperature );
      printf( "%s;%.1f;%.1f\n", time_str, sr->humidity, sr->temperature );
    }

}

int main(int argc, char** argv){

  int read_delay_ms = 3000;
  int num_retries = 3;

  if( 1 < argc ){
    read_delay_ms = atoi( argv[1] );
  }

  if( 2 < argc ){
    num_retries = atoi( argv[2] );
  }

  if(wiringPiSetup()==-1)
    exit(1);

  while(true){

    sensor_reading sr;

    if( dht22_read_val_with_retries( &sr, num_retries ) ){
      FILE *log = fopen("log.csv", "a");
      print_sensor_reading( log, &sr );
      fclose(log);
    }

    delay(read_delay_ms);

  }

  return 0;
}
