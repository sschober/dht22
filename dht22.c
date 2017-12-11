#include <wiringPi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_EDGE_TRANSITIONS 85
#define MAX_HOLD_TIME_MS 255
#define HOLD_TIME_1 60
#define DHT22_PIN 0
#define DHT22_READ_SEGMENT_SIZE 5

#define INITIAL_HOLD_LOW 18
#define INITIAL_HOLD_HIGH 40

bool is_checksum_valid(int *dht22_val){
  uint32_t sum = dht22_val[0];
  sum += dht22_val[1];
  sum += dht22_val[2];
  sum += dht22_val[3];
  return (sum & 0xFF) == dht22_val[4];
}

typedef struct {

  time_t time;
  float humidity;
  float temperature;

} sensor_reading;

bool dht22_read_val( sensor_reading *sr ){

  int dht22_val[DHT22_READ_SEGMENT_SIZE] = { 0, 0, 0, 0, 0 };
  uint8_t last_state = HIGH;
  uint8_t edge_transitions_witnessed = 0;

  // init dht22_val
  for(int i = 0; i < DHT22_READ_SEGMENT_SIZE; i++ ){
     dht22_val[i] = 0;
  }

  pinMode(DHT22_PIN,OUTPUT);

  digitalWrite(DHT22_PIN,LOW);
  delay(INITIAL_HOLD_LOW);

  digitalWrite(DHT22_PIN,HIGH);
  delayMicroseconds(INITIAL_HOLD_HIGH);

  pinMode(DHT22_PIN,INPUT);

  for( uint8_t edge_transitions = 0; edge_transitions < MAX_EDGE_TRANSITIONS ; edge_transitions++ ){

    uint8_t hold_time_ms = 0;

    // this is supposed to be edge detection
    while( digitalRead(DHT22_PIN) == last_state ){

      hold_time_ms++;
      delayMicroseconds(1);

      if( hold_time_ms == MAX_HOLD_TIME_MS ){
        break;
      }

    }

    if( hold_time_ms == MAX_HOLD_TIME_MS ){
      break;
    }

    last_state = digitalRead(DHT22_PIN);

    // top 3 transistions are ignored
    if( ( edge_transitions >= 4 ) && ( edge_transitions % 2 == 0 ) ){

      dht22_val[edge_transitions_witnessed/8]<<=1;

      if( hold_time_ms > HOLD_TIME_1 ){
        dht22_val[edge_transitions_witnessed/8]|=1;
      }

      edge_transitions_witnessed++;

    }

  }

  // verify cheksum and print the verified data
  if( edge_transitions_witnessed >= 39
      && is_checksum_valid( dht22_val )
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
int dht22_read_val_with_retries( sensor_reading *sr, uint8_t retries, uint16_t retry_delay_ms ){

  int result = -1;

  while( 0 < retries && false == ( result = dht22_read_val( sr ) ) ){
    printf( "retrying...\n" );
    retries--;
    delay( retry_delay_ms );
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

  uint16_t read_delay_ms = 3000;
  uint8_t num_retries = 3;
  uint16_t retry_delay_ms = 500;

  if( 1 < argc ){
    read_delay_ms = atoi( argv[1] );
  }

  if( 2 < argc ){
    num_retries = atoi( argv[2] );
  }

  if( 3 < argc ){
    retry_delay_ms = atoi( argv[3] );
  }

  if(wiringPiSetup()==-1){
    fprintf( stderr, "error setting up wiring Pi...\n" );
    exit(1);
  }

  while(true){

    sensor_reading sr;

    if( dht22_read_val_with_retries( &sr, num_retries, retry_delay_ms ) ){
      FILE *log = fopen("log.csv", "a");
      print_sensor_reading( log, &sr );
      fclose(log);
    }

    delay(read_delay_ms);

  }

  return 0;
}
