import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

def run_plot(ser, tare, calib_factor):
	# consts
	G = 9.80665

	# params
	MAX_POINTS = 100

	# data timeline
	data = deque([0]*MAX_POINTS, maxlen=MAX_POINTS)
	# timestamps = deque([0]*MAX_POINTS, maxlen=MAX_POINTS) # TODO: used in line.set_xdata

	# matplotlib
	fig, ax = plt.subplots()
	line, = ax.plot(data)
	ax.set_ylim(-1000, 1000000)

	# run
	def update(frame):
		while ser.in_waiting:
			try:
				z = int(ser.readline().decode().strip())
				force = (z - tare) * calib_factor
				data.append(force)
			except ValueError:
				pass
		line.set_ydata(data)
		line.set_xdata(range(len(data))) # TODO: Replace index with actual measurement timestamps
		return line,

	ani = animation.FuncAnimation(fig, update, interval=100)
	plt.show()

if __name__ == "__main__":
	raise Exception("Run python main.py plot -p PORT -b BAUD")