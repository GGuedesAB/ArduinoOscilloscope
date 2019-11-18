#include <Arduino.h>

#define N 300
uint8_t next_channel = 0x41;
uint16_t sample = 0;
int num_of_measurements = 0;
bool collect_data = false;

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

    bool is_full () {
      if (index == max_size-1) {
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

uint8_t switch_channel(uint8_t channel) {
  // We want one 2500 measurements
  /*   Channel tabel    *
   * CH0 -> Voltage     *
   * CH1 -> Current     */
  
  // This result must be decoded to correctly write on the MUX
  /* ADC0 -> 0x40 * 
   * ADC1 -> 0x41 */
  return channel ^ 0x01;
}

ISR(ADC_vect) {
  // Disable interrupts to avoid nested interrupts
  SREG ^= 0x80;
  sample = ADCL;
  sample += ADCH << 8;
  sample &= 0x3FF;
  ++num_of_measurements;
  collect_data = true;
  // Enable interrupts
  SREG ^= 0x80;
}

void setup() {
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  // Enable ADC noise reduction sleep mode, but do not allow the MCU to sleep right now
  //SMCR ^= SMCR;
  //SMCR |= 0x02;
  // Justify data to right and set ref to Vcc.
  ADMUX ^= ADMUX;
  ADMUX |= 0x40;
  // Configuring ADC Status Reg A
  // It must be enabled, not start a conversion yet
  // 101001111 -> Free running, interrupt enable, 128 division
  // Free running mode creates the challange of defining correctly which input will be multiplexed,
  // since it triggers it self with the last configuration.
  // That means it will capture twice the input it was configured in the setup method.
  ADCSRA ^= ADCSRA;
  ADCSRA |= 0X2F;
  // Configuring ADC Status Reg B
  // 000000000 -> ADC on, free running mode
  ADCSRB ^= ADCSRB;
  // Disable digital ADC
  DIDR0 = 0xFF;
  SREG |= 0x80;
  Serial.begin(115200);
  ADCSRA |= 0X80;
  ADCSRA |= 0X40;
}

void loop() {
  static ADC_RESULT voltage_buff;
  static ADC_RESULT current_buff;
  if (collect_data) {
    collect_data = false;
    ADCSRA ^= 0x08;
    SREG ^= 0x80;
    next_channel = switch_channel(next_channel);
    ADMUX = next_channel;
    if (num_of_measurements == 0) {
      // Do nothing
      _NOP();
    }
    else if (num_of_measurements == 2*N+1){
      num_of_measurements = -1;
      Serial.print("v|");
      voltage_buff.serial_transaction();
      Serial.print("i|");
      current_buff.serial_transaction();
    }
    else {
      if (ADMUX == 0x40){
        voltage_buff.push_value(sample);
      }
      else {
        current_buff.push_value(sample);
      }
    }
    // Enable interrupts
    SREG ^= 0x80;
    ADCSRA ^= 0x08;
  }
}