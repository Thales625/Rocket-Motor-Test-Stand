if __name__ == "__main__":
	raise Exception("Run python main.py -p PORT -b BAUD")

import json
from os.path import exists

SETTINGS_FILE = "settings.json"

DEFAULT_SETTINGS = {
	"port": None,
	"baud": 115200,
	"calib_mass": None,
	"calib_factor": None
}

def store_settings(settings):
	with open(SETTINGS_FILE, 'w') as f:
		json.dump(settings, f)
		# json.dump(settings, f, indent=4)

def load_settings():
	if not exists(SETTINGS_FILE):
		store_settings(DEFAULT_SETTINGS)
		return DEFAULT_SETTINGS.copy()

	with open(SETTINGS_FILE, 'r') as f:
		try:
			settings = json.load(f)
		except json.JSONDecodeError:
			print("Invalid json file, loading default settings.")
			settings = DEFAULT_SETTINGS.copy()

	return settings

def get_settings(args):
	write = False
	settings = load_settings()

	# overwrite settings
	if args.port:
		write = True
		settings["port"] = args.port

	if args.baud:
		write = True
		settings["baud"] = args.baud

	if args.command == "calibrate" and args.mass: # need to store the calibration mass
		write = True
		settings["calib_mass"] = args.mass

	# check None settings
	if settings["port"] is None:
		if write: store_settings(settings)
		raise Exception(
			"Porta serial não definida.\n\n"
			"Solução: Use a flag '-p' para especificar a porta.\n"
			"Exemplo de uso:\n"
			"    python main.py calibrate -p PORT -b BAUD"
		)

	if settings["baud"] is None:
		if write: store_settings(settings)
		raise Exception(
			"Baud rate não definido.\n\n"
			"Solução: Use a flag '-b' para especificar o baud rate.\n"
			"Exemplo de uso:\n"
			"    python main.py calibrate -p PORT -b BAUD"
		)
	
	if args.command == "calibrate": # need calibration mass
		if write: store_settings(settings)
		if args.mass is None:
			raise Exception(
				"Massa de calibração não definida.\n\n"
				"Solução: Use a flag '-m' para especificar a massa.\n"
				"Exemplo de uso:\n"
				"    python main.py calibrate -p PORT -b BAUD -m MASS"
			)
	
	if write: store_settings(settings)

	return settings