
from time import sleep

# consts
G = 9.80665

def calibrate(ser, tare, calib_mass):
	print(f"Coloque uma massa de {calib_mass}kg")

	input("Pressione Enter quando estiver pronto...")

	for i in range(3, 0, -1):
		print(f"Calibragem em {i} segundos")
		sleep(1)

	print("Calibrando...")

	while True:
		try:
			z = int(ser.readline().decode().strip()) # serial read
			print(z)
			return calib_mass * G / (z - tare)
		except ValueError:
			pass

if __name__ == "__main__":
	raise Exception("Run python main.py calibrate -p PORT -b BAUD -m MASS")