if __name__ == "__main__":
	import argparse
	import serial

	from settings import get_settings, store_settings

	# argument parsing
	parser = argparse.ArgumentParser(description="Load cell interface")
	subparsers = parser.add_subparsers(dest='command', required=False)

	# subparser for calibration
	parser_calib = subparsers.add_parser('calibrate', help='Run calibration')
	parser_calib.add_argument('-m', '--mass', type=float, help='Calibration mass')
	parser_calib.add_argument('-p', '--port', type=str, help='Serial port')
	parser_calib.add_argument('-b', '--baud', type=int, help='Baud rate')

	# subparser for plotting
	parser_plot = subparsers.add_parser('plot', help='Plot real-time data')
	parser_plot.add_argument('-p', '--port', type=str, help='Serial port')
	parser_plot.add_argument('-b', '--baud', type=int, help='Baud rate')

	# default fallback (run plot if no subcommand)
	parser.add_argument('-p', '--port', type=str, help='Serial port')
	parser.add_argument('-b', '--baud', type=int, help='Baud rate')

	args = parser.parse_args()
	
	settings = get_settings(args)

	# initialize serial
	ser = serial.Serial(settings["port"], settings["baud"])

	# TARE
	tare = 0
	while True:
		try:
			tare = int(ser.readline().decode().strip())
			break
		except ValueError:
			pass
	print("TARE:", tare)

	# commands
	if args.command == "calibrate":
		from calib import calibrate

		calib_factor = calibrate(ser, tare, settings["calib_mass"])
		settings["calib_factor"] = calib_factor
		store_settings(settings)

		print(f"Novo fator de calibração salvo: {calib_factor:.6f}")

	else: # plot (or default fallback)
		if settings["calib_factor"] is None:
			raise Exception("Fator de calibração não encontrado.\nRun: python main.py calibrate -p PORT -b BAUD -m MASS")

		from plot import run_plot

		run_plot(ser, tare, settings["calib_factor"])