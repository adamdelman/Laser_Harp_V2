#ifndef LASER_HARP_LASERHARPSTRING_H
#define LASER_HARP_LASERHARPSTRING_H

#include "NewPing.h"
#include "Arduino.h"

#define LDR_PIN 1
#define TRIGGER_PIN    9 // Arduino pin tied to trigger pin on ping sensor.
#define ECHO_PIN       10 // Arduino pin tied to echo pin on ping sensor.

#define PING_REST_INTERVAL_MS 100 // How frequently are we going to send out a ping (in milliseconds). 50ms would be 20 times a second.
#define MIN_DISTANCE_CM 10
#define MAX_DISTANCE_CM 200 // Maximum distance_cm we want to ping for (in centimeters). Maximum sensor distance_cm is rated at 400-500cm.
#define LIGHT_CALIBRATION_ROUNDS 10
#define LIGHT_OFFSET 2
#define DISTANCE_MEASUREMENT_ROUNDS 20
#define NUM_OF_OCTAVES 2

class LaserHarpString {
public:
    LaserHarpString(int min_distance_cm, int max_cm_distance, int light_calibration_rounds,
                    int distance_measurement_rounds, int num_of_octaves);

    LaserHarpString() : LaserHarpString(MIN_DISTANCE_CM, MAX_DISTANCE_CM,
                                        LIGHT_CALIBRATION_ROUNDS,
                                        DISTANCE_MEASUREMENT_ROUNDS, NUM_OF_OCTAVES) {}

    void check_note();

private:
    void calibrate_light_sensor();

    void measure_distance();

    int get_current_octave_index();

    int get_light_level();

    bool is_ping_activated();

    bool is_note_activated();

    void send_note_on_signal(int octave_index);

    void send_note_off_signal();
    
    int m_min_distance_cm;
    int m_max_distance_cm;
    int m_last_note_octave_index;
    int m_num_of_octaves;
    int m_octave_range_cm;
    int m_light_calibration_rounds;
    int m_distance_measurement_rounds;
    unsigned long m_next_ping_time_ms;
    int m_base_light_level;
    long m_average_distance_cm;
    bool m_previous_note_active_check_status;
    NewPing m_sonar = NewPing((uint8_t) TRIGGER_PIN, (uint8_t) ECHO_PIN, m_max_distance_cm);

};

#endif //LASER_HARP_LASERHARPSTRING_H
