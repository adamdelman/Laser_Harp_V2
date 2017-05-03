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
    m_last_note_octave_index = -1;
};

void LaserHarpString::calibrate_light_sensor() {
    delay(1000);
    int light_level = 0;
    for (int counter = 0; counter < LIGHT_CALIBRATION_ROUNDS; counter++) {
        delay(100);
        light_level += 1023 - analogRead(LDR_PIN);
    }
    m_base_light_level = (light_level / LIGHT_CALIBRATION_ROUNDS) + LIGHT_OFFSET;
//    Serial.print("\nLight sensor threshold is :");
//    Serial.println(m_base_light_level);
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
    for (int index = 0; index < m_distance_measurement_rounds; index++) {
        distance_cm = m_sonar.ping_cm(MAX_DISTANCE_CM);
        if (!distance_cm) {
            index--;
        } else {
            average_distance_cm += distance_cm;
        }
    }
    m_average_distance_cm = average_distance_cm / m_distance_measurement_rounds;
//    Serial.print("\nAverage distance CM: ");
//    Serial.println(m_average_distance_cm);
}

int LaserHarpString::get_current_octave_index() {
    int octave = 1;
    while ((octave * m_octave_range_cm) < m_average_distance_cm) {
        octave++;
    }
//    Serial.println("OCTAVE ");
//    Serial.println(octave - 1);
    return octave - 1;
}

int LaserHarpString::get_light_level() {
    int light_level = 1023 - analogRead(LDR_PIN);
//    Serial.print("\nLight level: ");
//    Serial.println(light_level);
    return light_level;

}

bool LaserHarpString::is_ping_activated() {
    bool ping_activated =
            (m_average_distance_cm >= m_min_distance_cm) && (m_average_distance_cm <= m_max_distance_cm * 0.6);
//    Serial.print("\nPing activation: ");
//    Serial.println(ping_activated);
    return ping_activated;
}

bool LaserHarpString::is_note_activated() {
    int light_level = get_light_level();
    measure_distance();
    bool ping_activated = is_ping_activated();
    bool light_activated = light_level > m_base_light_level;
//    Serial.print("\nLight activation: ");
//    Serial.println(light_activated);
    return light_activated || ping_activated;

}

void LaserHarpString::send_note_on_signal(int octave_index) {
    Serial.write(1);
    Serial.write(octave_index);
    Serial.flush();
}


void LaserHarpString::send_note_off_signal() {
    Serial.write(0);
    Serial.write(0);
    Serial.flush();
}


void LaserHarpString::check_note() {
    bool note_activated = is_note_activated();
    if (note_activated) {
        int octave_index = get_current_octave_index();
        if ((!m_previous_note_active_check_status) && (octave_index != m_last_note_octave_index)) {
            send_note_on_signal(octave_index);
        }
        m_last_note_octave_index = octave_index;

    } else {
        if (m_previous_note_active_check_status) {
            send_note_off_signal();
        }

    }
    m_previous_note_active_check_status = note_activated;
}