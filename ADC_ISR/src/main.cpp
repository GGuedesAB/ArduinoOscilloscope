#include <Arduino.h>

#define N 400
uint8_t next_channel = 0x41;
uint16_t sample = 0;
int num_of_measurements = 0;
bool collect_data = false;
bool buffer_ready = false;
bool conversion_complete = false;

class ADC_RESULT {
  uint16_t data [N];
  int max_size;
  int index;
  private:
    bool is_empty () {
      if (index == -1) {
        return true;
      }
      else {
        return false;
      }
    }

  public:
    ADC_RESULT(){
      max_size = (int) N;
      index = -1;
    }

    bool is_full () {
      if (index == max_size-1) {
        return true;
      }
      else {
        return false;
      }
    }

    void push_value (uint16_t value){
      if (index < max_size-1)
        data[++index] = value;
    }

    uint16_t pop_value (){
      if (index >= 0){
        return data[index--];
      }
      else {
        index = -1;
        return index;
      }
    }

    void serial_transaction (){
      while (! is_empty()){
        Serial.print(pop_value());
        Serial.print("|");
      }
      Serial.print("\n");
    }
};

void change_measure_channel() {
  // We want one 2500 measurements
  /*   Channel tabel    *
   * CH0 -> Voltage     *
   * CH1 -> Current     *
   * CH2 -> Temperature *
   * CH3 -> Light       */
  
  // This result must be decoded to correctly write on the MUX
  /* ADC0 -> 0x40 * 
   * ADC1 -> 0x41 */
  if (num_of_measurements < 2*N){
    ADMUX = ADMUX ^ 0x01;
  }
  else if (num_of_measurements == 2*N) {
    ADMUX = 0x42;
  }
  else if (num_of_measurements == 2*N+1) {
    ADMUX = 0x43;
  }
  else {
    num_of_measurements = 0;
    buffer_ready = true;
    ADMUX = 0x40;
  }
}

void start_ad_conversion () {
  conversion_complete = false;
  ADCSRA |= (1<<ADSC);
}

void capture_sampled_value () {
  // Disable interrupts to avoid nested interrupts
  SREG &= ~0x80;
  TCNT1 = 0;
  while (!conversion_complete) {
    SREG |= 0x80;
    TCNT1 = 0;
  }
  sample = ADCL;
  sample += ADCH << 8;
  sample &= 0x3FF;
  collect_data = true;
  num_of_measurements++;
  // Enable interrupts
  SREG |= 0x80;
}

ISR(ADC_vect) {
  conversion_complete = true;
}

ISR(TIMER1_COMPA_vect) {
  capture_sampled_value();
}

void setup() {
  Serial.begin(115200);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  // Justify data to right and set ref to Vcc.
  
  ADMUX ^= ADMUX;
  ADMUX |= 0x40;
  
  // Configuring ADC Status Reg A
  // It must be enabled, not start a conversion yet
  // 101001111 -> Interrupt enable, auto trigger, 32 division -> 26us conversion time
  /* Free running mode creates the challange of defining correctly which input will be multiplexed, *
   * since it triggers it self with the last configuration.                                         *
   * That means it will capture twice the input it was configured in the setup method.              */
  
  ADCSRA ^= ADCSRA;
  ADCSRA |= (1<<ADEN);
  ADCSRA |= (1<<ADATE);
  ADCSRA |= (1<<ADIE);
  ADCSRA |= (1<<ADPS2);
  //ADCSRA |= (1<<ADPS1);
  ADCSRA |= (1<<ADPS0);
  
  // ADCSRA should equal 0X2F
  // Configuring ADC Status Reg B
  // 000000101 -> ADC on, interrupt when TimerCounter1 matches B = 0
  // Right now I never want it to match!!!!
  ADCSRB ^= ADCSRB;
  ADCSRB |= (1<<ADTS2);
  ADCSRB |= (1<<ADTS0);
  
  // Disable digital ADC
  DIDR0 = 0xFF;
  
  ADCSRA |= (1<<ADSC);
  // Now I need to implement timing Compare value + count speed
  // Count up to 1000 @ 16MHz -> 16kS/s
  TCCR1A ^= TCCR1A;
  TCCR1B ^= TCCR1B;
  TCCR1B |= (1<<WGM12);
  TCCR1B |= (1<<CS10);
  TIMSK1 |= (1<<OCIE1A);
  TCNT1 = 0;
  OCR1A = 1000;
  OCR1B = 4000;
  SREG |= 0x80;
}

void loop() {
  static ADC_RESULT voltage_buff;
  static ADC_RESULT current_buff;
  static int temperature = 0;
  static int light = 0;
  if (collect_data) {
    SREG ^= 0x80;
    collect_data = false;
    int old_ADMUX = ADMUX;
    change_measure_channel();
    start_ad_conversion();
    if (old_ADMUX == 0x40){
      voltage_buff.push_value(sample);
    }
    else if (old_ADMUX == 0x41){
      current_buff.push_value(sample);
    }
    else if (old_ADMUX == 0x42){
      temperature = sample;
    }
    else if (old_ADMUX == 0x43) {
      light = sample;
    }
    if (buffer_ready) {
      buffer_ready = false;
      Serial.print("V|");
      voltage_buff.serial_transaction();
      Serial.print("I|");
      current_buff.serial_transaction();
      Serial.print("T|");
      Serial.println(temperature);
      Serial.print("L|");
      Serial.println(light);
      Serial.println(">");
    }
    SREG ^= 0x80;
  }
}