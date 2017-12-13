#include <wiringPi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_EDGE_TRANSITIONS 83
#define MAX_HOLD_TIME_MS 255
#define HOLD_TIME_1 60
#define DHT22_PIN 0
#define DHT22_READ_SEGMENT_SIZE 5

#define INITIAL_HOLD_LOW 18
#define INITIAL_HOLD_HIGH 40

bool is_checksum_valid(int *bit_array){
  uint32_t sum = bit_array[0];
  sum += bit_array[1];
  sum += bit_array[2];
  sum += bit_array[3];
  return (sum & 0xFF) == bit_array[4];
}

typedef struct {

  time_t time;
  float humidity;
  float temperature;

} sensor_reading;

void signal_read_intent(){

  pinMode(DHT22_PIN,OUTPUT);

  digitalWrite(DHT22_PIN,LOW);
  delay(INITIAL_HOLD_LOW);

  digitalWrite(DHT22_PIN,HIGH);
  delayMicroseconds(INITIAL_HOLD_HIGH);

}

uint8_t read_bit_array( int *bit_array ){

  uint8_t last_state = HIGH;
  uint8_t edge_transitions_witnessed = 0;

  for( uint8_t edge_transitions = 0; edge_transitions < MAX_EDGE_TRANSITIONS; edge_transitions++ ){

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

      bit_array[edge_transitions_witnessed/8] <<= 1;

      if( hold_time_ms > HOLD_TIME_1 ){
        bit_array[edge_transitions_witnessed/8] |= 1;
      }

      edge_transitions_witnessed++;

    }

  }

  return edge_transitions_witnessed;

}

bool acquire_sensor_reading( sensor_reading *sr ){

  int bit_array[DHT22_READ_SEGMENT_SIZE] = { 0, 0, 0, 0, 0 };
  uint8_t bit_array_read = 0;

  signal_read_intent();

  pinMode(DHT22_PIN,INPUT);

  bit_array_read = read_bit_array( bit_array );

  // verify cheksum and print the verified data
  if( bit_array_read >= 39
      && is_checksum_valid( bit_array )
    ){
    // valid run, checksum checks out

    sr->time = time(NULL);
    sr->humidity = (256 * (float) bit_array[0] + (float) bit_array[1] )/10.0;
    sr->temperature = (256 * (float) (bit_array[2] & 0x7F) + (float) bit_array[3])/10.0;

    return true;

  }
  // invalid run
  return false;
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
int acquire_sensor_reading_with_retries( sensor_reading *sr, uint8_t retries, uint16_t retry_delay_ms ){

  bool result = false;

  while( 0 < retries && ! ( result = acquire_sensor_reading( sr ) ) ){
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

    if( acquire_sensor_reading_with_retries( &sr, num_retries, retry_delay_ms ) ){

      FILE *log = fopen("log.csv", "a");
      print_sensor_reading( log, &sr );
      fclose(log);

    }

    delay(read_delay_ms);

  }

  return 0;
}
