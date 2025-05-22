import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

from time import sleep

"""
TODO:
	- Pass argument
		-Pass SERIAL_PORT as an argument when executing the Python script, e.g.:
			python main.py -p COM5
		-Also allow specifying the BAUD_RATE, e.g.:
			python main.py -p COM5 -b 115200

	- Create a separate script `calib.py` to calibrate the load cell.
		- This script should:
			• Read raw serial data while a known mass is applied.
			• Compute the calibration constant C.
			• Save the C value to a plain text file (e.g., 'calib.txt').

		- In the main script:
			• Read the saved C value from 'calib.txt'.
			• Use this value to convert raw readings to physical force units.
"""

# serial params
SERIAL_PORT = '/dev/ttyUSB0' # 'COM5'
BAUD_RATE = 115200

# params
MAX_POINTS = 100

# consts
G = 9.80665

# initialize serial
ser = serial.Serial(SERIAL_PORT, BAUD_RATE)

# data timeline
data = deque([0]*MAX_POINTS, maxlen=MAX_POINTS)
# timestamps = deque([0]*MAX_POINTS, maxlen=MAX_POINTS) # TODO: used in line.set_xdata

# matplotlib
fig, ax = plt.subplots()
line, = ax.plot(data)
ax.set_ylim(-1000, 1000000)

# TARE
while True:
	try:
		z = int(ser.readline().decode().strip())
		TARE = z
		print("TARE:", TARE)
		break
	except ValueError:
		pass

# calibatrion
M = 1
print(f"Coloque uma massa de {M}kg")

input("Pressione Enter quando estiver pronto...")

for i in range(3, 1, -1):
	print(f"Calibragem em {i} segundos")
	sleep(1)

print(f"Calibrando para massa de {M}kg")

while True:
	try:
		z = int(ser.readline().decode().strip())
		C = M * G / (z - TARE)
		print("C:", C)
		break
	except ValueError:
		pass

# run
def update(frame):
	while ser.in_waiting:
		try:
			z = int(ser.readline().decode().strip())
			force = (z - TARE) * C
			data.append(force)
		except ValueError:
			pass
	line.set_ydata(data)
	line.set_xdata(range(len(data))) # TODO: Replace index with actual measurement timestamps
	return line,

ani = animation.FuncAnimation(fig, update, interval=100)
plt.show()