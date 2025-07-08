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
	times = times[10:]
	values = values[10:]

	# esp time analysis
	# plt.plot([i for i in range(len(times))], times, 'o', c='r')
	# plt.plot([i for i in range(len(times))], times, 'o', c='r')

	i_arr = [i for i in range(len(times)-1)]
	plt.plot(i_arr, [times[i+1]-times[i] for i in i_arr], 'o', c='r')
	plt.show()

	# value process
	t0 = 16000
	t1 = 36500

	v0 = np.interp(t0, times, values)
	v1 = np.interp(t1, times, values)

	deriv = (v1-v0) / (t1-t0)

	fixed_values = [values[i]-deriv*(times[i]-t0) - v0 for i in range(len(times))]

	# plot
	plt.plot(times, values, label="Raw data")
	# plt.plot(times, values, 'o', label="Raw data")
	# plt.plot(times, fixed_values, label="Processed data")

	# plt.axvline(t0, linestyle="dotted", c="r")
	# plt.axvline(t1, linestyle="dotted", c="r")
	# plt.axhline(v0, linestyle="dotted", c="r")

	plt.xlabel(r"Time $\left[ms\right]$")
	plt.ylabel(r"Sensor Reading $\left[\frac{N}{N}\right]$")
	plt.title("Static Fire")
	plt.grid(True)
	plt.tight_layout()
	plt.legend()
	plt.show()