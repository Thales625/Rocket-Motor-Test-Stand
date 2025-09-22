def index_of(array, value):
	for i, v in enumerate(array):
		if v >= value:
			return i
	return None

if __name__ == "__main__":
	import argparse
	from pathlib import Path
	import matplotlib.pyplot as plt
	import numpy as np

	# argument parsing
	parser = argparse.ArgumentParser(description="Data Analysis")

	parser.add_argument(
		"file",
		type=Path,
		nargs='?',
		default=Path("./datalog.txt"),
		help="Path to datalog file (ex: ~/DATALOG.txt)"
	)

	args = parser.parse_args()

	# solve path
	file_path = args.file.expanduser()

	# data
	times = []
	values = []

	# read file
	try:
		with open(file_path, 'r') as f:
			for line in f:
				line = line.strip()

				if not line: continue

				part = line.split()

				if len(part) < 2: continue

				times.append(int(part[0]))
				values.append(int(part[1]))
	except Exception as e:
		print(f"Error reading file: {e}")
		exit()
	
	# process data

	# ignore first 10 readings
	times = times[10:]
	values = values[10:]

	# esp time analysis
	# i_arr = [i for i in range(len(times)-1)]
	# plt.plot(i_arr, [times[i+1]-times[i] for i in i_arr], 'o', c='r')
	# plt.show()

	# value process
	t0 = 50000
	t1 = times[-1]

	i0 = index_of(times, t0)
	i1 = index_of(times, t1)

	v0 = values[i0]
	v1 = values[i1]

	deriv = (v1-v0) / (t1-t0)

	times = times[i0:i1]
	values = values[i0:i1]

	values = [values[i]-deriv*(times[i]-t0) - v0 for i in range(len(times))]

	mean = np.mean(values)
	var = np.sum((values - mean)**2) / (len(values)-1)

	print("Mean: ", mean)
	print("Min: ", np.min(values))
	print("Max: ", np.max(values))
	print("Var: ", var)

	# plot
	plt.plot(times, values, label="data")
	plt.plot(times[:-1], np.diff(values), label="deriv")

	plt.xlabel(r"Time $\left[ms\right]$")
	plt.ylabel(r"Sensor Reading $\left[\frac{N}{N}\right]$")
	plt.title("Static Fire")
	plt.grid(True)
	plt.tight_layout()
	plt.legend()
	plt.show()