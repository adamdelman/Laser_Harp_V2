#include "LaserHarpString.h"

LaserHarpString::LaserHarpString(int min_distance_cm, int max_distance_cm, int light_calibration_rounds,
                                 int distance_measurement_rounds, int num_of_octaves) {
    m_min_distance_cm = min_distance_cm;
    m_max_distance_cm = max_distance_cm;
    m_num_of_octaves = num_of_octaves;
    m_light_calibration_rounds = light_calibration_rounds;
    m_distance_measurement_rounds = distance_measurement_rounds;
    m_previous_note_active_check_status = false;
    calibrate_light_sensor();
    m_next_ping_time_ms = millis();
    m_octave_range_cm = (m_max_distance_cm - m_min_distance_cm) / m_num_of_octaves;
#ifdef DEBUG_HARP
    Serial.print("Octave range in CM: ");
    Serial.println(m_octave_range_cm);
#endif
};

void LaserHarpString::calibrate_light_sensor() {
    delay(1000);
    long light_level = 0;
    for (int counter = 0; counter < LIGHT_CALIBRATION_ROUNDS; counter++) {
        int light_reading = 1023 - analogRead(LDR_PIN);
#ifdef DEBUG_HARP_SENSOR
        Serial.print("Light sensor calibration reading is: ");
        Serial.println(light_reading);
#endif
        light_level += light_reading;
    }
    m_base_light_level = (light_level / LIGHT_CALIBRATION_ROUNDS) + LIGHT_OFFSET;
#ifdef DEBUG_HARP_SENSOR
    Serial.print("Light sensor threshold is: ");
    Serial.println(m_base_light_level);
#endif
}

void LaserHarpString::measure_distance() {
    long distance_cm;
    long average_distance_cm = 0;
    unsigned long current_time_ms = millis();
    while (current_time_ms < m_next_ping_time_ms) {
        delay(PING_REST_INTERVAL_MS);
        current_time_ms = millis();
    }
    m_next_ping_time_ms = current_time_ms + PING_REST_INTERVAL_MS;// Set the next ping time.
    int average_factor = m_distance_measurement_rounds;
    for (int index = 0; index < m_distance_measurement_rounds; index++) {
        distance_cm = m_sonar.ping_cm(MAX_DISTANCE_CM);
//        Serial.print("Distance CM: ");
//        Serial.println(distance_cm);
        if (!distance_cm) {
            if (average_factor > 1) {
                average_factor--;
            }
        } else {
            average_distance_cm += distance_cm;
        }
    }
    m_average_distance_cm = average_distance_cm / average_factor;
//#ifdef DEBUG_HARP_SENSOR
//    Serial.print("Average distance CM: ");
//    Serial.println(m_average_distance_cm);
//#endif

}

int LaserHarpString::get_current_octave_index() {
    int octave = 0;
    if (m_average_distance_cm == 0) {
        return m_last_note_octave_index;
    } else {
        while ((octave * m_octave_range_cm) < m_average_distance_cm) {
            octave++;
        }
//#ifdef DEBUG_HARP
//        Serial.print("Current octave index is ");
//        Serial.println(octave - 1);
//#endif
        if (octave > 0) {
            octave--;
        }
        return octave;
    }
}

int LaserHarpString::get_light_level() {
    int light_level = 1023 - analogRead(LDR_PIN);
//#ifdef DEBUG_HARP_SENSOR
//    Serial.print("Light level: ");
//    Serial.println(light_level);
//#endif
    return light_level;

}

bool LaserHarpString::is_ping_activated() {
    bool ping_activated =
            (m_average_distance_cm >= m_min_distance_cm) && (m_average_distance_cm <= m_max_distance_cm * 0.6);
#ifdef DEBUG_HARP_SENSOR
//    Serial.print("Ping activation: ");
//    Serial.println(ping_activated);
    if (ping_activated) {
        Serial.println("Ping activated.");
    }
#endif
    return ping_activated;
}

bool LaserHarpString::is_light_activated() {
    int light_level = get_light_level();
    bool light_activated = light_level > m_base_light_level;
#ifdef DEBUG_HARP_SENSOR
//    Serial.print("Light activation: ");
//    Serial.println(light_activated);
    if (light_activated) {
        Serial.println("Light activated.");
    }
#endif
    return light_activated;
}

bool LaserHarpString::is_note_activated() {
    measure_distance();
    bool ping_activated = is_ping_activated();
    bool light_activated = is_light_activated();
#ifdef DEBUG_HARP_SENSOR
    if (light_activated || ping_activated){
        Serial.println("Note activated.");
    }
    else {
        Serial.println("Note not activated.");
    }
#endif
    return light_activated || ping_activated;
}

void LaserHarpString::send_note_on_signal(int octave_index) {
#ifdef DEBUG_HARP
    Serial.print("Sending note on signal for octave index ");
    Serial.println(octave_index);
#else
    Serial.write(1);
    Serial.write(octave_index);
    Serial.flush();
#endif
    if ((m_previous_note_active_check_status) && (m_last_note_octave_index != -1) && (octave_index != m_last_note_octave_index)) {
#ifdef DEBUG_HARP
        Serial.print("Turning off previous activate note for octave ");
        Serial.println(m_last_note_octave_index);
#endif
        send_note_off_signal(m_last_note_octave_index);
    }
    m_last_note_octave_index = octave_index;

}


void LaserHarpString::send_note_off_signal(int octave_index) {
#ifdef DEBUG_HARP
    Serial.print("Sending note off signal for octave index ");
    Serial.println(octave_index);
#else
    Serial.write(0);
    Serial.write(octave_index);
    Serial.flush();
#endif
}


void LaserHarpString::check_note() {
    bool note_activated = is_note_activated();
    int octave_index = get_current_octave_index();
    if (note_activated) {
        if ((!m_previous_note_active_check_status) || (octave_index != m_last_note_octave_index)) {
            send_note_on_signal(octave_index);
        }
    } else {
        if (m_previous_note_active_check_status) {
            send_note_off_signal(octave_index);
        }
    }
    m_previous_note_active_check_status = note_activated;
}