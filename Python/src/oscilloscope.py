import serial
import os
import matplotlib.pyplot as plot
import matplotlib.animation as animation
import time

class Oscilloscope ():
    def __init__ (self):
        self.port = os.path.join('/dev','ttyS5')
        self.baud = 115200
        self.number_of_samples = 400
        self.zeroes = [0] * self.number_of_samples
        self.voltage = self.zeroes
        self.current = self.zeroes
        self.temperature = self.zeroes
        self.light = self.zeroes
        self.unformatted_voltage = ''
        self.unformatted_current = ''
        self.unformatted_temperature = ''
        self.unformatted_light = ''
        self.data_range = list(range(0, self.number_of_samples))
        try:
            self.ser = serial.Serial(self.port, self.baud)
        except:
            print ('ERROR: Could not connect to serial port ' + self.port)
            exit(1)
        
    def get_serial_data (self):
        self.data = self.ser.read_until('>'.encode('utf-8'))
        try:
            self.data = self.data.decode('utf-8')
            self.data = self.data.rsplit('\n')
            self.data.remove('\r')
            self.unformatted_voltage = self.data[0]
            self.unformatted_current = self.data[1]
            self.unformatted_temperature = self.data[2]
            self.unformatted_light = self.data[3]
        except:
            raise ValueError
        
    def get_voltage (self):
        if self.unformatted_voltage:
            identifier = 'V'
            self.voltage = self.unformatted_voltage.rsplit('|')
            self.voltage.remove(identifier)
            self.voltage.pop()
            result = list(map(int, self.voltage))
        else:
            result = self.zeroes
        yield result

    def get_current (self):
        if self.unformatted_current:
            identifier = 'I'
            self.current = self.unformatted_current.rsplit('|')
            self.current.remove(identifier)
            self.current.pop()
            result = list(map(int, self.current))
        else:
            result = self.zeroes
        yield result

    def get_instant_power (self):
        instant_power = [voltage*current for voltage,current in zip(self.voltage, self.current)]
        yield instant_power

    # def get_temperature (self):
    #     if self.unformatted_temperature:
    #         self.temperature = self.unformatted_temperature.rsplit('|')
    #         self.temperature = self.temperature[1].rsplit('\r')
    #         self.temperature.pop()
    #         result = list(map(int, self.temperature))
    #         result = result*self.number_of_samples
    #     else:
    #         result = self.zeroes
    #     yield result

    # def get_light (self):
    #     if self.unformatted_light:
    #         self.light = self.unformatted_light.rsplit('|')
    #         self.light = self.light[1].rsplit('\r')
    #         self.light.pop()
    #         result = list(map(int, self.light))
    #         result = result*self.number_of_samples
    #     else:
    #         result = self.zeroes
    #     yield result

    def gen_voltage_line (self, data):
        try:
            self.get_serial_data()
        except ValueError:
            self.unformatted_voltage = ''
            self.unformatted_current = ''
            self.unformatted_temperature = ''
            self.unformatted_light = ''

        if (len(data) != self.number_of_samples):
            print ('WARNING: Communication error!')
            data = self.zeroes
        line1.set_data(self.data_range, data)

    def gen_current_line (self, data):
        if (len(data) != self.number_of_samples):
            print ('WARNING: Communication error!')
            data = self.zeroes
        line2.set_data(self.data_range, data)

    def gen_temperature_line (self, data):
        if (len(data) != self.number_of_samples):
            print ('WARNING: Communication error!')
            data = self.zeroes
        line3.set_data(self.data_range, data)
        
    #def gen_light_line (self, data):
    #    if (len(data) != self.number_of_samples):
    #        print ('WARNING: Communication error!')
    #        data = self.zeroes
    #    line4.set_data(self.data_range, data)

if __name__ == "__main__":
    my_osc = Oscilloscope()
    print ('Started monitor.')
    fig, ((ax1, ax2), (ax3,ax4)) = plot.subplots(2,2)
    
    line1, = ax1.plot(my_osc.data_range, my_osc.zeroes)
    
    line2, = ax2.plot(my_osc.data_range, my_osc.zeroes)
    
    line3, = ax3.plot(my_osc.data_range, my_osc.zeroes)
    
    line4, = ax4.plot(my_osc.data_range, my_osc.zeroes)
    
    ax1.set_xlim(0, 400)
    ax1.set_ylim(-1, 1100)
    ax1.set_title('Tensão (V)')
    
    ax2.set_xlim(0, 400)
    ax2.set_ylim(-1, 1100)
    ax2.set_title('Corrente (A)')
    
    ax3.set_xlim(0, 400)
    ax3.set_ylim(-1, 1100)
    ax3.set_title('Potência instantânea (W)')

    ani1 = animation.FuncAnimation(fig=fig, func=my_osc.gen_voltage_line, frames=my_osc.get_voltage, interval=50)
    ani2 = animation.FuncAnimation(fig=fig, func=my_osc.gen_current_line, frames=my_osc.get_current, interval=50)
    ani3 = animation.FuncAnimation(fig=fig, func=my_osc.gen_temperature_line, frames=my_osc.get_temperature, interval=50)

    plot.show()